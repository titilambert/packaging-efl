#include "evas_engine.h"
#include <sys/mman.h>

static unsigned int 
_evas_drm_crtc_buffer_get(int fd, int crtc_id)
{
   drmModeCrtc *crtc;
   unsigned int id;

   if (!(crtc = drmModeGetCrtc(fd, crtc_id))) return 0;
   id = crtc->buffer_id;
   drmModeFreeCrtc(crtc);
   return id;
}

static void 
_evas_drm_outbuf_page_flip(int fd EINA_UNUSED, unsigned int seq EINA_UNUSED, unsigned int tv_sec EINA_UNUSED, unsigned int tv_usec EINA_UNUSED, void *data)
{
   Outbuf *ob;

   /* get the output buffer from data */
   if (!(ob = data)) return;

   /* DBG("Page Flip Event"); */

   ob->priv.pending_flip = EINA_FALSE;
   ob->priv.last = ob->priv.curr;
   ob->priv.curr = (ob->priv.curr + 1) % ob->priv.num;
}

/* static void  */
/* _evas_drm_outbuf_vblank(int fd EINA_UNUSED, unsigned int frame EINA_UNUSED, unsigned int sec EINA_UNUSED, unsigned int usec EINA_UNUSED, void *data) */
/* { */
/*    Outbuf *ob; */

/*    if (!(ob = data)) return; */

/*    DBG("VBlank Event"); */
/* } */

static Eina_Bool 
_evas_drm_outbuf_planes_setup(Outbuf *ob, drmModePlaneResPtr pres)
{
   drmModePlanePtr dplane;
   Plane *oplane;
   unsigned int p = 0;
   unsigned int f = 0;

   for (p = 0; p < pres->count_planes; p++)
     {
        /* try to get this plane */
        if (!(dplane = drmModeGetPlane(ob->priv.fd, pres->planes[p])))
          continue;

        /* try to allocate space for our plane */
        if (!(oplane = 
              malloc(sizeof(Plane)  + 
                     ((sizeof(unsigned int)) * dplane->count_formats))))
          {
             drmModeFreePlane(dplane);
             continue;
          }

        oplane->crtcs = dplane->possible_crtcs;
        oplane->id = dplane->plane_id;
        oplane->num_formats = dplane->count_formats;
        memcpy(oplane->formats, dplane->formats, 
               dplane->count_formats * sizeof(dplane->formats[0]));

        DBG("Plane %d, %d %d", p, dplane->x, dplane->y);
        DBG("\tFB: %d", dplane->fb_id);
        DBG("\tCrtc: %d, %d %d", dplane->crtc_id, 
            dplane->crtc_x, dplane->crtc_y);

        DBG("\tSupported Formats");
        for (f = 0; f < dplane->count_formats; f++)
          {
             DBG("\t\t%C%C%C%C", (dplane->formats[f] & 0xFF), 
                 ((dplane->formats[f] >> 8) & 0xFF),
                 ((dplane->formats[f] >> 16) & 0xFF), 
                 ((dplane->formats[f] >> 24) & 0xFF));
          }

        /* free this plane */
        drmModeFreePlane(dplane);

        /* append this plane */
        ob->priv.planes = eina_list_append(ob->priv.planes, oplane);
     }

   if (eina_list_count(ob->priv.planes) < 1) return EINA_FALSE;
   return EINA_TRUE;
}

Eina_Bool 
evas_drm_outbuf_setup(Outbuf *ob)
{
   drmModeRes *res;
   drmModeConnector *conn;
   drmModePlaneResPtr pres;
   drmModeEncoder *enc;
   drmModeModeInfo crtc_mode;
   int i = 0;

   /* check for valid Output buffer */
   if ((!ob) || (ob->priv.fd < 0)) return EINA_FALSE;

   /* setup drmHandleEvent context */
   memset(&ob->priv.ctx, 0, sizeof(ob->priv.ctx));
   ob->priv.ctx.version = DRM_EVENT_CONTEXT_VERSION;
   ob->priv.ctx.page_flip_handler = _evas_drm_outbuf_page_flip;
   /* ob->priv.ctx.vblank_handler = _evas_drm_outbuf_vblank; */

   /* try to get drm resources */
   if (!(res = drmModeGetResources(ob->priv.fd)))
     {
        CRI("Could not get drm resources: %m");
        return EINA_FALSE;
     }

   /* loop the connectors */
   for (; i < res->count_connectors; ++i)
     {
        int crtc_id = -1;
        int m = 0;

        /* try to get this connector */
        if (!(conn = drmModeGetConnector(ob->priv.fd, res->connectors[i])))
          {
             WRN("Could not get drm connector %d: %m", i);
             continue;
          }

        /* make sure this connector is actually connected */
        if (conn->connection != DRM_MODE_CONNECTED) 
          {
             /* free connector resources */
             drmModeFreeConnector(conn);
             continue;
          }

        /* make sure it has modes */
        if (conn->count_modes == 0)
          {
             /* free connector resources */
             drmModeFreeConnector(conn);
             continue;
          }

        /* record the connector id */
        ob->priv.conn = conn->connector_id;

        if ((enc = drmModeGetEncoder(ob->priv.fd, conn->encoder_id)))
          {
             drmModeCrtc *crtc;

             if ((crtc = drmModeGetCrtc(ob->priv.fd, enc->crtc_id)))
               {
                  crtc_id = enc->crtc_id;
                  if (crtc->mode_valid) crtc_mode = crtc->mode;
                  drmModeFreeCrtc(crtc);
               }

             drmModeFreeEncoder(enc);
          }

        /* record the crtc id */
        ob->priv.crtc = crtc_id;

        /* get the current framebuffer */
        ob->priv.fb = _evas_drm_crtc_buffer_get(ob->priv.fd, crtc_id);

        /* spew out connector properties for testing */
        /* drmModePropertyPtr props; */
        /* for (m = 0; m < conn->count_props; m++) */
        /*   { */
        /*      props = drmModeGetProperty(ob->priv.fd, conn->props[m]); */
        /*      if (!props) continue; */
        /*      DBG("Property Name: %s", props->name); */
        /*   } */

        memset(&ob->priv.mode, 0, sizeof(ob->priv.mode));
        for (m = 0; m < conn->count_modes; m++)
          {
             DBG("Output Available Mode: %d: %d %d %d", ob->priv.conn, 
                 conn->modes[m].hdisplay, conn->modes[m].vdisplay, 
                 conn->modes[m].vrefresh);
             if (!memcmp(&crtc_mode, &conn->modes[m], sizeof(crtc_mode)))
               {
                  /* record the current mode */
                  memcpy(&ob->priv.mode, &conn->modes[m], sizeof(ob->priv.mode));
                  break;
               }
          }

        if ((!ob->priv.mode.hdisplay) && (crtc_mode.clock != 0))
          memcpy(&ob->priv.mode, &crtc_mode, sizeof(ob->priv.mode));

        DBG("Output Current Mode: %d: %d %d", ob->priv.conn, 
            ob->priv.mode.hdisplay, ob->priv.mode.vdisplay);

        if ((ob->priv.mode.hdisplay != conn->modes[0].hdisplay) || 
            (ob->priv.mode.vdisplay != conn->modes[0].vdisplay))
          {
             /* set new crtc mode */
             drmModeSetCrtc(ob->priv.fd, ob->priv.crtc, ob->priv.fb, 0, 0, 
                            &ob->priv.conn, 1, &ob->priv.mode);
          }

        /* free connector resources */
        drmModeFreeConnector(conn);

        break;
     }

   /* get any plane resource from the card */
   pres = drmModeGetPlaneResources(ob->priv.fd);

   /* if we have at least one plane, set it up */
   if (pres->count_planes > 0)
     {
        if (!_evas_drm_outbuf_planes_setup(ob, pres))
          WRN("Could not setup hardware planes");
     }

   /* free plane resources */
   drmModeFreePlaneResources(pres);

   /* free drm resources */
   drmModeFreeResources(res);

   return EINA_TRUE;
}

void 
evas_drm_outbuf_framebuffer_set(Outbuf *ob, Buffer *buffer)
{
   int ret;

   /* validate params */
   if ((!ob) || (!buffer)) return;

   /* DBG("Drm Framebuffer Set: %d", buffer->fb); */

   buffer->valid = EINA_FALSE;
   ret = drmModeSetCrtc(ob->priv.fd, ob->priv.crtc, buffer->fb, 0, 0, 
                        &ob->priv.conn, 1, &ob->priv.mode);

   if (ret) ERR("Failed to set crtc: %m");
   else buffer->valid = EINA_TRUE;
}

Eina_Bool 
evas_drm_framebuffer_create(int fd, Buffer *buffer, int depth)
{
   struct drm_mode_create_dumb carg;
   struct drm_mode_destroy_dumb darg;
   struct drm_mode_map_dumb marg;

   /* check for valid info */
   if (fd < 0) return EINA_FALSE;

   /* try to create a dumb buffer */
   memset(&carg, 0, sizeof(carg));
   carg.width = buffer->w;
   carg.height = buffer->h;
   carg.bpp = depth;

   if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &carg) < 0)
     {
        ERR("Could not create dumb buffer: %m");
        return EINA_FALSE;
     }

   buffer->stride = carg.pitch;
   buffer->size = carg.size;
   buffer->handle = carg.handle;

   DBG("Buffer: %d %d", buffer->w, buffer->h);
   DBG("Buffer Stride: %d", buffer->stride);
   DBG("Buffer Size: %d", buffer->size);

   /* try to create a framebuffer object */
   /* FIXME: Hardcoded bpp */
   if (drmModeAddFB(fd, buffer->w, buffer->h, 24, depth, buffer->stride, 
                    buffer->handle, &buffer->fb))
     {
        ERR("Could not create framebuffer object: %m");
        goto add_err;
     }

   DBG("Creating dumb buffer: %d %d %d %d", buffer->fb, 
       buffer->w, buffer->h, depth);

   /* try to mmap the buffer */
   memset(&marg, 0, sizeof(marg));
   marg.handle = buffer->handle;
   if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &marg))
     {
        ERR("Could not map dumb buffer: %m");
        goto map_err;
     }

   /* do actual mmap of memory */
   buffer->data = 
     mmap(NULL, buffer->size, (PROT_READ | PROT_WRITE), 
          MAP_SHARED, fd, marg.offset);
   if (buffer->data == MAP_FAILED)
     {
        ERR("Could not mmap dumb buffer: %m");
        goto map_err;
     }

   /* clear memory */
   memset(buffer->data, 0, buffer->size);

   return EINA_TRUE;

map_err:
   /* remove the framebuffer */
   drmModeRmFB(fd, buffer->fb);

add_err:
   /* destroy buffer */
   memset(&darg, 0, sizeof(darg));
   darg.handle = buffer->handle;
   drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &darg);

   return EINA_FALSE;
}

void 
evas_drm_framebuffer_destroy(int fd, Buffer *buffer)
{
   struct drm_mode_destroy_dumb darg;

   /* check for valid info */
   if (fd < 0) return;

   /* unmap the buffer data */
   if (buffer->data) munmap(buffer->data, buffer->size);

   /* remove the framebuffer */
   drmModeRmFB(fd, buffer->fb);

   /* destroy buffer */
   memset(&darg, 0, sizeof(darg));
   darg.handle = buffer->handle;
   drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &darg);
}

Eina_Bool 
evas_drm_framebuffer_send(Outbuf *ob, Buffer *buffer)
{
   /* check for valid Output buffer */
   if ((!ob) || (ob->priv.fd < 0)) return EINA_FALSE;

   /* check for valid buffer */
   if (!buffer) return EINA_FALSE;

   if (ob->vsync)
     {
        if (drmModePageFlip(ob->priv.fd, ob->priv.crtc, 
                            buffer->fb, DRM_MODE_PAGE_FLIP_EVENT, ob) < 0)
          {
             ERR("Cannot flip crtc for connector %u: %m", ob->priv.conn);
             return EINA_FALSE;
          }

        ob->priv.pending_flip = EINA_TRUE;

        while (ob->priv.pending_flip)
          drmHandleEvent(ob->priv.fd, &ob->priv.ctx);
     }
   else
     {
        /* NB: We don't actually need to do this if we are not vsync
         * because we are drawing directly to the buffer anyway.
         * If we enable the sending of buffer to crtc, it causes vsync */

        /* send this buffer to the crtc */
        /* evas_drm_outbuf_framebuffer_set(ob, buffer); */

        /* increment buffer we are using */
        ob->priv.curr = (ob->priv.curr + 1) % ob->priv.num;
     }

   return EINA_TRUE;
}
