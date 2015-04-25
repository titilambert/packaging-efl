#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ecore_drm_private.h"
#include <dlfcn.h>

static void 
_ecore_drm_device_cb_page_flip(int fd EINA_UNUSED, unsigned int frame EINA_UNUSED, unsigned int sec EINA_UNUSED, unsigned int usec EINA_UNUSED, void *data)
{
   Ecore_Drm_Output *output;

   DBG("Drm Page Flip Event");

   if (!(output = data)) return;

   if (output->pending_flip)
     {
        ecore_drm_output_fb_release(output, output->current);
        output->current = output->next;
        output->next = NULL;
     }

   output->pending_flip = EINA_FALSE;
   if (!output->pending_vblank) ecore_drm_output_repaint(output);
}

static void 
_ecore_drm_device_cb_vblank(int fd EINA_UNUSED, unsigned int frame EINA_UNUSED, unsigned int sec EINA_UNUSED, unsigned int usec EINA_UNUSED, void *data)
{
   Ecore_Drm_Sprite *sprite;
   Ecore_Drm_Output *output;

   DBG("Drm VBlank Event");

   if (!(sprite = data)) return;

   output = sprite->output;
   output->pending_vblank = EINA_FALSE;

   ecore_drm_output_fb_release(output, sprite->current_fb);
   sprite->current_fb = sprite->next_fb;
   sprite->next_fb = NULL;

   if (!output->pending_flip) _ecore_drm_output_frame_finish(output);
}

static Eina_Bool 
_ecore_drm_device_cb_event(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   Ecore_Drm_Device *dev;
   drmEventContext ctx;

   if (!(dev = data)) return ECORE_CALLBACK_RENEW;

   DBG("Drm Device Event");

   memset(&ctx, 0, sizeof(ctx));

   ctx.version = DRM_EVENT_CONTEXT_VERSION;
   ctx.page_flip_handler = _ecore_drm_device_cb_page_flip;
   ctx.vblank_handler = _ecore_drm_device_cb_vblank;

   drmHandleEvent(dev->drm.fd, &ctx);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool 
_ecore_drm_device_cb_idle(void *data)
{
   Ecore_Drm_Device *dev;
   Ecore_Drm_Output *output;
   Eina_List *l;

   if (!(dev = data)) return ECORE_CALLBACK_CANCEL;

   EINA_LIST_FOREACH(dev->outputs, l, output)
     {
        output->need_repaint = EINA_TRUE;
        if (output->repaint_scheduled) continue;
        _ecore_drm_output_repaint_start(output);
     }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @defgroup Ecore_Drm_Device_Group Device manipulation functions
 * 
 * Functions that deal with finding, opening, closing, and otherwise using 
 * the DRM device itself.
 */

/**
 * Find a drm device in the system.
 *
 * @param name The name of the device to find. If NULL, this function will 
 *             search for the default drm device.
 * @param seat The name of the seat where this device may be found. If NULL, 
 *             this function will use a default seat name 'seat0'.
 * 
 * @return An opaque Ecore_Drm_Device structure representing the card.
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Ecore_Drm_Device *
ecore_drm_device_find(const char *name, const char *seat)
{
   Ecore_Drm_Device *dev = NULL;
   Eina_Bool found = EINA_FALSE;
   Eina_Bool platform = EINA_FALSE;
   Eina_List *devs, *l;
   const char *device;

   /* try to get a list of drm devics */
   if (!(devs = eeze_udev_find_by_type(EEZE_UDEV_TYPE_DRM, name)))
     return NULL;

   DBG("Find Drm Device: %s", name);

   EINA_LIST_FOREACH(devs, l, device)
     {
        const char *devpath;
        const char *devseat;
        const char *devparent;

        if (!(devpath = eeze_udev_syspath_get_devpath(device)))
          continue;

        DBG("Found Drm Device");
        DBG("\tDevice: %s", device);
        DBG("\tDevpath: %s", devpath);

        if ((name) && (strcmp(name, devpath))) goto cont;

        if (!(devseat = eeze_udev_syspath_get_property(device, "ID_SEAT")))
          devseat = eina_stringshare_add("seat0");

        if ((seat) && (strcmp(seat, devseat))) 
          goto cont;
        else if (strcmp(devseat, "seat0"))
          goto cont;

        devparent = eeze_udev_syspath_get_parent_filtered(device, "pci", NULL);
        if (!devparent)
          {
             devparent = 
               eeze_udev_syspath_get_parent_filtered(device, "platform", NULL);
             platform = EINA_TRUE;
          }

        if (devparent)
          {
             if (!platform)
               {
                  const char *id;

                  if ((id = eeze_udev_syspath_get_sysattr(devparent, "boot_vga")))
                    {
                       if (!strcmp(id, "1")) found = EINA_TRUE;
                       eina_stringshare_del(id);
                    }
               }
             else
               found = EINA_TRUE;

             eina_stringshare_del(devparent);
          }

cont:
        eina_stringshare_del(devpath);
        if (found) break;
     }

   if (!found) goto out;

   if ((dev = calloc(1, sizeof(Ecore_Drm_Device))))
     {
        dev->drm.name = eeze_udev_syspath_get_devpath(device);
        dev->drm.path = eina_stringshare_add(device);

        dev->id = eeze_udev_syspath_get_sysnum(device);

        dev->seat = eeze_udev_syspath_get_property(device, "ID_SEAT");
        if (!dev->seat) dev->seat = eina_stringshare_add("seat0");

        dev->vt = 0;
        dev->format = 0;
        dev->use_hw_accel = EINA_FALSE;
        dev->session = NULL;

        DBG("Using Drm Device: %s", dev->drm.name);
     }

out:
   EINA_LIST_FREE(devs, device)
     eina_stringshare_del(device);

   return dev;
}

/**
 * Free an Ecore_Drm_Device
 *
 * This function will cleanup and free any previously allocated Ecore_Drm_Device.
 * 
 * @param dev The Ecore_Drm_Device to free
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI void 
ecore_drm_device_free(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Output *output;

   /* check for valid device */
   if (!dev) return;

   /* free outputs */
   EINA_LIST_FREE(dev->outputs, output)
     ecore_drm_output_free(output);

   /* free crtcs */
   if (dev->crtcs) free(dev->crtcs);

   /* free device name */
   if (dev->drm.name) eina_stringshare_del(dev->drm.name);

   /* free device path */
   if (dev->drm.path) eina_stringshare_del(dev->drm.path);

   /* free device seat */
   if (dev->seat) eina_stringshare_del(dev->seat);

   /* free session */
   free(dev->session);

   /* free structure */
   free(dev);
}

/**
 * Open an Ecore_Drm_Device
 *
 * This function will open an existing Ecore_Drm_Device for use.
 * 
 * @param dev The Ecore_Drm_Device to try and open
 * 
 * @return EINA_TRUE on success, EINA_FALSE on failure
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Eina_Bool 
ecore_drm_device_open(Ecore_Drm_Device *dev)
{
   uint64_t caps;

   /* check for valid device */
   if ((!dev) || (!dev->drm.name)) return EINA_FALSE;

   /* DRM device node is needed immediately to keep going. */
   dev->drm.fd = 
     _ecore_drm_launcher_device_open_no_pending(dev->drm.name, O_RDWR);
   if (dev->drm.fd < 0) return EINA_FALSE;

   DBG("Opened Device %s : %d", dev->drm.name, dev->drm.fd);

   if (!drmGetCap(dev->drm.fd, DRM_CAP_TIMESTAMP_MONOTONIC, &caps))
     {
        if (caps == 1)
          dev->drm.clock = CLOCK_MONOTONIC;
        else
          dev->drm.clock = CLOCK_REALTIME;
     }
   else
     {
        ERR("Could not get TIMESTAMP_MONOTONIC device capabilities: %m");
     }

   /* Without DUMB_BUFFER we can't do software rendering on DRM. Fail without it
    * until we have rock solid hardware accelerated DRM on all drivers */
   if (drmGetCap(dev->drm.fd, DRM_CAP_DUMB_BUFFER, &caps) < 0 || !caps)
     {
        ERR("Could not get DUMB_BUFFER device capabilities: %m");
        return EINA_FALSE;
     }

   /* try to create xkb context */
   if (!(dev->xkb_ctx = xkb_context_new(0)))
     {
        ERR("Failed to create xkb context: %m");
        return EINA_FALSE;
     }

   dev->drm.hdlr = 
     ecore_main_fd_handler_add(dev->drm.fd, ECORE_FD_READ, 
                               _ecore_drm_device_cb_event, dev, NULL, NULL);

   dev->drm.idler = 
     ecore_idle_enterer_add(_ecore_drm_device_cb_idle, dev);

   return EINA_TRUE;
}

/**
 * Close an Ecore_Drm_Device
 *
 * This function will close a previously opened Ecore_Drm_Device
 * 
 * @param dev The Ecore_Drm_Device to free
 * 
 * @return EINA_TRUE on success, EINA_FALSE on failure
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Eina_Bool 
ecore_drm_device_close(Ecore_Drm_Device *dev)
{
   /* check for valid device */
   if (!dev) return EINA_FALSE;

   /* close xkb context */
   if (dev->xkb_ctx) xkb_context_unref(dev->xkb_ctx);

   if (dev->drm.hdlr) ecore_main_fd_handler_del(dev->drm.hdlr);
   dev->drm.hdlr = NULL;

   _ecore_drm_launcher_device_close(dev->drm.name, dev->drm.fd);

   /* reset device fd */
   dev->drm.fd = -1;

   return EINA_TRUE;
}

/**
 * Get if a given Ecore_Drm_Device is master
 * 
 * This function will check if the given drm device is set to master
 * 
 * @param dev The Ecore_Drm_Device to check
 * 
 * @return EINA_TRUE if device is master, EINA_FALSE otherwise
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Eina_Bool 
ecore_drm_device_master_get(Ecore_Drm_Device *dev)
{
   drm_magic_t mag;

   /* check for valid device */
   if ((!dev) || (dev->drm.fd < 0)) return EINA_FALSE;

   /* get if we are master or not */
   if ((drmGetMagic(dev->drm.fd, &mag) == 0) && 
       (drmAuthMagic(dev->drm.fd, mag) == 0))
     return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * Set a given Ecore_Drm_Device to master
 * 
 * This function will attempt to set a given drm device to be master
 * 
 * @param dev The Ecore_Drm_Device to set
 * 
 * @return EINA_TRUE on success, EINA_FALSE on failure
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Eina_Bool 
ecore_drm_device_master_set(Ecore_Drm_Device *dev)
{
   /* check for valid device */
   if ((!dev) || (dev->drm.fd < 0)) return EINA_FALSE;

   DBG("Set Master On Fd: %d", dev->drm.fd);

   drmSetMaster(dev->drm.fd);

   return EINA_TRUE;
}

/**
 * Tell a given Ecore_Drm_Device to stop being master
 * 
 * This function will attempt to ask a drm device to stop being master
 * 
 * @param dev The Ecore_Drm_Device to set
 * 
 * @return EINA_TRUE on success, EINA_FALSE on failure
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Eina_Bool 
ecore_drm_device_master_drop(Ecore_Drm_Device *dev)
{
   /* check for valid device */
   if ((!dev) || (dev->drm.fd < 0)) return EINA_FALSE;

   DBG("Drop Master On Fd: %d", dev->drm.fd);

   drmDropMaster(dev->drm.fd);

   return EINA_TRUE;
}

/**
 * Get the file descriptor of Ecore_Drm_Device
 *
 * This function will get the file descriptor of drm device
 *
 * @param dev The Ecore_Drm_Device to get
 *
 * @return fd value on success, -1 on failure
 *
 * @ingroup Ecore_Drm_Device_Group
 *
 */
EAPI int 
ecore_drm_device_fd_get(Ecore_Drm_Device *dev)
{
   if (!dev) return -1;
   return dev->drm.fd;
}

/**
 * Set the window of Ecore_Drm_Device
 *
 * This function will set the window for given drm device
 *
 * @param dev The Ecore_Drm_Device for which window is set
 * @param window The window to set
 *
 * @ingroup Ecore_Drm_Device_Group
 *
 * @since 1.10
 */
EAPI void 
ecore_drm_device_window_set(Ecore_Drm_Device *dev, unsigned int window)
{
   /* check for valid device */
   if ((!dev) || (dev->drm.fd < 0)) return;

   dev->window = window;
}

/**
 * Get the name of the Ecore_Drm_Device
 *
 * This function will return the name of Ecore_Drm_Device
 *
 * @param dev The Ecore_Drm_Device to get name
 *
 * @return device name on success, NULL on failure
 *
 * @ingroup Ecore_Drm_Device_Group
 *
 * @since 1.10
 */
EAPI const char *
ecore_drm_device_name_get(Ecore_Drm_Device *dev)
{
   /* check for valid device */
   if ((!dev) || (dev->drm.fd < 0)) return NULL;

   return dev->drm.name;
}
