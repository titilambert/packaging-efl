#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_xlib_image.h"

static void
evas_xlib_image_update(void *data EINA_UNUSED, void *image, int x, int y, int w, int h)
{
   RGBA_Image *im = image;
   Native *n = im->native.data;
   char *pix;
   int bpl, rows, bpp;

   if (ecore_x_image_get(n->exim, n->pixmap, 0, 0, x, y, w, h))
     {
        pix = ecore_x_image_data_get(n->exim, &bpl, &rows, &bpp);
        if (!ecore_x_image_is_argb32_get(n->exim))
          {
             if (!im->image.data)
               im->image.data = (DATA32 *)malloc(im->cache_entry.w * im->cache_entry.h * sizeof(DATA32));
             Ecore_X_Colormap colormap = ecore_x_default_colormap_get(ecore_x_display_get(), ecore_x_default_screen_get());
             ecore_x_image_to_argb_convert(pix, bpp, bpl, colormap, n->visual,
                                           x, y, w, h,
                                           im->image.data, (w * sizeof(int)), 0, 0);
          }
        else
          {
             im->image.data = (DATA32 *)pix;
          }
     }
}

static void
_native_bind_cb(void *data, void *image, int x, int y, int w, int h)
{
   RGBA_Image *im = image;
   Native *n = im->native.data;

   if ((n) && (n->ns.type == EVAS_NATIVE_SURFACE_X11))
     {
        evas_xlib_image_update(data, image, x, y, w, h);
     }
}

static void
_native_free_cb(void *data EINA_UNUSED, void *image)
{
   RGBA_Image *im = image;
   Native *n = im->native.data;

   if (n->exim)
     {
        ecore_x_image_free(n->exim);
        n->exim = NULL;
     }
   n->visual = NULL;

   im->native.data        = NULL;
   im->native.func.data   = NULL;
   im->native.func.bind   = NULL;
   im->native.func.free   = NULL;
   im->image.data         = NULL;
   free(n);
}

void *
evas_xlib_image_native_set(void *data, void *image, void *native)
{
   RGBA_Image *im = image;
   Evas_Native_Surface *ns = native;
   Native  *n = NULL;
   Ecore_X_Image *exim = NULL;
   Visual  *vis = NULL;
   Pixmap   pm = 0;
   int w, h, depth;

   if ((ns) && (ns->type == EVAS_NATIVE_SURFACE_X11))
     {
        vis = ns->data.x11.visual;
        pm = ns->data.x11.pixmap;

        depth = ecore_x_drawable_depth_get(pm);

        w = im->cache_entry.w;
        h = im->cache_entry.h;

        n = calloc(1, sizeof(Native));
        if (!n) return NULL;

        exim = ecore_x_image_new(w, h, vis, depth);
        if (!exim)
          {
             ERR("ecore_x_image_new failed.");
             free(n);
             return NULL;
          }

        memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));
        n->pixmap = pm;
        n->visual = vis;
        n->exim = exim;
        im->native.data = n;
        im->native.func.data = NULL;
        im->native.func.bind = _native_bind_cb;
        im->native.func.free = _native_free_cb;

        evas_xlib_image_update(data, image, 0, 0, w, h);
     }
   return im;
}
