#include "evas_engine.h"
#include <sys/mman.h>

static void
_evas_drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
   Buffer *buffer = data;
   struct gbm_device *gbm = gbm_bo_get_device(bo);

   if (buffer->fb)
     drmModeRmFB(gbm_device_get_fd(gbm), buffer->fb);
}

static int
_evas_drm_crtc_find(int fd, drmModeRes *res, drmModeConnector *conn)
{
   int crtc = -1;
   drmModeEncoder *enc = NULL;

   /* if this connector already has an encoder, get it */
   if (conn->encoder_id) enc = drmModeGetEncoder(fd, conn->encoder_id);

   /* if this encoder already has a crtc, lets try to use that */
   if ((enc) && (enc->crtc_id)) crtc = enc->crtc_id;

   if (crtc < 0)
     {
        int i = 0, c = 0;

        /* if this connector has no encoder, we need to find one */
        for (; i < conn->count_encoders; ++i)
          {
             /* try to get this encoder */
             if (!(enc = drmModeGetEncoder(fd, conn->encoders[i])))
               continue;

             /* loop global crtcs */
             for (; c < res->count_crtcs; ++c)
               {
                  /* does this crtc work with this encoder ? */
                  if (!(enc->possible_crtcs & (1 << c))) continue;

                  /* FIXME: We could be more proactive here and check that
                   * nobody else is using this crtc */

                  /* if it works, let's use it */
                  crtc = res->crtcs[c];
                  break;
               }

             if (crtc >= 0) break;
          }
     }

   /* free the encoder */
   if (enc) drmModeFreeEncoder(enc);

   return crtc;
}

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
   Buffer *buff;

   /* get the output buffer from data */
   if (!(ob = data)) return;

   buff = &(ob->priv.buffer[ob->priv.curr]);
   gbm_surface_release_buffer(ob->surface, buff->bo);

   ob->priv.pending_flip = EINA_FALSE;
   ob->priv.curr = (ob->priv.curr + 1) % ob->priv.num;
}

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
evas_drm_gbm_init(Evas_Engine_Info_GL_Drm *info, int w, int h)
{
   if (!info) return EINA_FALSE;
   if (info->info.fd < 0)  return EINA_FALSE;

   if (!(info->info.gbm = gbm_create_device(info->info.fd)))
     return EINA_FALSE;

   if (!(info->info.surface = 
         gbm_surface_create(info->info.gbm, w, h,
                            info->info.format, info->info.flags)))
     {
        gbm_device_destroy(info->info.gbm);
        info->info.gbm = NULL;
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

Eina_Bool
evas_drm_gbm_shutdown(Evas_Engine_Info_GL_Drm *info)
{
   if (!info) return EINA_TRUE;

   if (info->info.surface)
     {
        gbm_surface_destroy(info->info.surface);
        info->info.surface = NULL;
     }
   if (info->info.gbm)
     {
        gbm_device_destroy(info->info.gbm);
        info->info.gbm = NULL;
     }

   return EINA_TRUE;
}

Eina_Bool
evas_drm_outbuf_setup(Outbuf *ob)
{
   drmModeRes *res;
   drmModeConnector *conn;
   drmModePlaneResPtr pres;
   int i = 0;

   /* check for valid Output buffer */
   if ((!ob) || (ob->priv.fd < 0)) return EINA_FALSE;

   /* setup drmHandleEvent context */
   memset(&ob->priv.ctx, 0, sizeof(ob->priv.ctx));
   ob->priv.ctx.version = DRM_EVENT_CONTEXT_VERSION;
   ob->priv.ctx.page_flip_handler = _evas_drm_outbuf_page_flip;

   /* try to get drm resources */
   if (!(res = drmModeGetResources(ob->priv.fd)))
     {
        CRI("Could not get drm resources: %m");
        return EINA_FALSE;
     }

   /* loop the connectors */
   for (; i < res->count_connectors; ++i)
     {
        int crtc = -1;
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

        /* try to find a crtc for this connector */
        if ((crtc = _evas_drm_crtc_find(ob->priv.fd, res, conn)) < 0)
          {
             /* free connector resources */
             drmModeFreeConnector(conn);
             continue;
          }

        /* record the connector id */
        ob->priv.conn = conn->connector_id;

        /* record the crtc id */
        ob->priv.crtc = crtc;

        /* get the current framebuffer */
        ob->priv.fb = _evas_drm_crtc_buffer_get(ob->priv.fd, crtc);

        /* record the current mode */
        memcpy(&ob->priv.mode, &conn->modes[0], sizeof(ob->priv.mode));

        for (m = 0; m < conn->count_modes; m++)
          {
             DBG("Output Available Mode: %d: %d %d %d", ob->priv.conn,
                 conn->modes[m].hdisplay, conn->modes[m].vdisplay,
                 conn->modes[m].vrefresh);

             /* try to find a mode which matches the requested size */
             if ((conn->modes[m].hdisplay == ob->w) &&
                 (conn->modes[m].vdisplay == ob->h) &&
                 (conn->modes[m].vrefresh == 60))
               {
                  memcpy(&ob->priv.mode, &conn->modes[m],
                         sizeof(ob->priv.mode));
               }
          }

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
   uint32_t handles[4], pitches[4], offsets[4];
   uint32_t width, height;
   uint32_t format;

   /* validate params */
   if ((!ob) || (!buffer)) return;

   if (buffer->valid) return;

   width = gbm_bo_get_width(buffer->bo);
   height = gbm_bo_get_height(buffer->bo);
   buffer->stride = gbm_bo_get_stride(buffer->bo);
   buffer->handle = gbm_bo_get_handle(buffer->bo).u32;
   buffer->size = buffer->stride * height;
   format = gbm_bo_get_format(buffer->bo);

   handles[0] = buffer->handle;
   pitches[0] = buffer->stride;
   offsets[0] = 0;

   ret = drmModeAddFB2(ob->priv.fd, width, height, format, handles,
                       pitches, offsets, &(buffer->fb), 0);
   if (ret)
     {
        ret = drmModeAddFB(ob->priv.fd, width, height, 24, 32,
                           buffer->stride, buffer->handle, &(buffer->fb));
     }
   if (ret) ERR("Failed to AddFB: %m");

   ret = drmModeSetCrtc(ob->priv.fd, ob->priv.crtc, buffer->fb, 0, 0,
                        &ob->priv.conn, 1, &ob->priv.mode);
   if (ret) ERR("Failed to set crtc: %m");

   gbm_bo_set_user_data(buffer->bo, buffer, _evas_drm_fb_destroy_callback);

   buffer->valid = EINA_TRUE;
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
