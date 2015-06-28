void
SCALE_FUNC(RGBA_Image *src, RGBA_Image *dst, int dst_clip_x, int dst_clip_y, int dst_clip_w, int dst_clip_h, DATA32 mul_col, int render_op, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   DATA32  *dst_ptr;
   int      src_w, src_h, dst_w, dst_h;

   if ((!src->image.data) || (!dst->image.data)) return;
   if (!(RECTS_INTERSECT(dst_region_x, dst_region_y, dst_region_w, dst_region_h,
                         0, 0, dst->cache_entry.w, dst->cache_entry.h))) return;
   if (!(RECTS_INTERSECT(src_region_x, src_region_y, src_region_w, src_region_h,
                         0, 0, src->cache_entry.w, src->cache_entry.h))) return;

   src_w = src->cache_entry.w;
   src_h = src->cache_entry.h;
   dst_w = dst->cache_entry.w;
   dst_h = dst->cache_entry.h;

   if (dst_clip_x < 0)
     {
        dst_clip_w += dst_clip_x;
        dst_clip_x = 0;
     }
   if (dst_clip_y < 0)
     {
        dst_clip_h += dst_clip_y;
        dst_clip_y = 0;
     }

   if ((dst_clip_w <= 0) || (dst_clip_h <= 0)) return;
   if ((dst_clip_x + dst_clip_w) > dst_w) dst_clip_w = dst_w - dst_clip_x;
   if ((dst_clip_y + dst_clip_h) > dst_h) dst_clip_h = dst_h - dst_clip_y;

   if (dst_clip_x < dst_region_x)
     {
	dst_clip_w += dst_clip_x - dst_region_x;
	dst_clip_x = dst_region_x;
     }
   if ((dst_clip_x + dst_clip_w) > (dst_region_x + dst_region_w))
     dst_clip_w = dst_region_x + dst_region_w - dst_clip_x;
   if (dst_clip_y < dst_region_y)
     {
	dst_clip_h += dst_clip_y - dst_region_y;
	dst_clip_y = dst_region_y;
     }
   if ((dst_clip_y + dst_clip_h) > (dst_region_y + dst_region_h))
     dst_clip_h = dst_region_y + dst_region_h - dst_clip_y;

   if ((src_region_w <= 0) || (src_region_h <= 0) ||
       (dst_region_w <= 0) || (dst_region_h <= 0) ||
       (dst_clip_w <= 0) || (dst_clip_h <= 0))
     return;

   /* sanitise x */
   if (src_region_x < 0)
     {
	dst_region_x -= (src_region_x * dst_region_w) / src_region_w;
	dst_region_w += (src_region_x * dst_region_w) / src_region_w;
	src_region_w += src_region_x;
	src_region_x = 0;
     }
   if (src_region_x >= src_w) return;
   if ((src_region_x + src_region_w) > src_w)
     {
	dst_region_w = (dst_region_w * (src_w - src_region_x)) / (src_region_w);
	src_region_w = src_w - src_region_x;
     }
   if (dst_region_w <= 0) return;
   if (src_region_w <= 0) return;
   if (dst_clip_x < 0)
     {
	dst_clip_w += dst_clip_x;
	dst_clip_x = 0;
     }
   if (dst_clip_w <= 0) return;
   if (dst_clip_x >= dst_w) return;
   if (dst_clip_x < dst_region_x)
     {
	dst_clip_w += (dst_clip_x - dst_region_x);
	dst_clip_x = dst_region_x;
     }
   if ((dst_clip_x + dst_clip_w) > dst_w)
     {
	dst_clip_w = dst_w - dst_clip_x;
     }
   if (dst_clip_w <= 0) return;

   /* sanitise y */
   if (src_region_y < 0)
     {
	dst_region_y -= (src_region_y * dst_region_h) / src_region_h;
	dst_region_h += (src_region_y * dst_region_h) / src_region_h;
	src_region_h += src_region_y;
	src_region_y = 0;
     }
   if (src_region_y >= src_h) return;
   if ((src_region_y + src_region_h) > src_h)
     {
	dst_region_h = (dst_region_h * (src_h - src_region_y)) / (src_region_h);
	src_region_h = src_h - src_region_y;
     }
   if (dst_region_h <= 0) return;
   if (src_region_h <= 0) return;
   if (dst_clip_y < 0)
     {
	dst_clip_h += dst_clip_y;
	dst_clip_y = 0;
     }
   if (dst_clip_h <= 0) return;
   if (dst_clip_y >= dst_h) return;
   if (dst_clip_y < dst_region_y)
     {
	dst_clip_h += (dst_clip_y - dst_region_y);
	dst_clip_y = dst_region_y;
     }
   if ((dst_clip_y + dst_clip_h) > dst_h)
     {
	dst_clip_h = dst_h - dst_clip_y;
     }
   if (dst_clip_h <= 0) return;

   /* some maximum region sizes to avoid insane calc point tables */
   if (dst_clip_w > 65536) return;
   if (dst_clip_h > 65536) return;
   if (dst_region_w > (65536 * 1024)) return;
   if (dst_region_h > (65536 * 1024)) return;

   /* figure out dst jump
    * NB: Unused currently, so commented out */
//   dst_jump = dst_w - dst_clip_w;

   /* figure out dest start ptr */
   dst_ptr = dst->image.data + dst_clip_x + (dst_clip_y * dst_w);

   if (mask_ie)
     {
        // Adjust clipping info
        if (EINA_UNLIKELY((dst_clip_x - mask_x) < 0))
          dst_clip_x = mask_x;
        if (EINA_UNLIKELY((dst_clip_y - mask_y) < 0))
          dst_clip_y = mask_y;
        if (EINA_UNLIKELY((dst_clip_x - mask_x + dst_clip_w) > (int)mask_ie->cache_entry.w))
          dst_clip_w = mask_ie->cache_entry.w - dst_clip_x + mask_x;
        if (EINA_UNLIKELY((dst_clip_y - mask_y + dst_clip_h) > (int)mask_ie->cache_entry.h))
          dst_clip_h = mask_ie->cache_entry.h - dst_clip_y + mask_y;
     }

/* FIXME:
 *
 * things to do later for speedups:
 *
 * break upscale into 3 cases (as listed below - up:up, 1:up, up:1)
 *
 * break downscale into more cases (as listed below)
 *
 * roll func (blend/copy/cultiply/cmod) code into inner loop of scaler.
 * (578 fps vs 550 in mmx upscale in evas demo - this means probably
 *  a good 10-15% speedup over the func call, but means massively larger
 *  code)
 *
 * anything involving downscaling has no mmx equivalent code and maybe the
 * C could do with a little work.
 *
 * ---------------------------------------------------------------------------
 *
 * (1 = no scaling (1:1 ratio), + = scale up, - = scale down)
 * (* == fully optimised mmx, # = fully optimised C)
 *
 * h:v mmx C
 *
 * 1:1 *   #
 *
 * +:+ *   #
 * 1:+ *   #
 * +:1 *   #
 *
 * 1:-
 * -:1
 * +:-
 * -:+
 * -:-
 *
 */

   /* if 1:1 scale */
   if ((dst_region_w == src_region_w) &&
       (dst_region_h == src_region_h))
     {
#include "evas_scale_smooth_scaler_noscale.c"
     }
   else
     {
	/* scaling up only - dont need anything except original */
//	if ((!dc->anti_alias) || ((dst_region_w >= src_region_w) && (dst_region_h >= src_region_h)))
	if (((dst_region_w >= src_region_w) && (dst_region_h >= src_region_h)))
	  {
#include "evas_scale_smooth_scaler_up.c"
	     return;
	  }
	else
	  /* scaling down... funkiness */
	  {
#include "evas_scale_smooth_scaler_down.c"
	     return;
	  }
     }
}
