{
   int Cy, j;
   DATA32 *dptr, *pix, *pbuf, **yp;
   DATA8 *mask;
   int r, g, b, a, rr, gg, bb, aa;
   int *xp, xap, yap, pos;
   //int dyy, dxx;
   int w = dst_clip_w;
   int y;

   dptr = dst_ptr;
   pos = (src_region_y * src_w) + src_region_x;
   //dyy = dst_clip_y - dst_region_y;
   //dxx = dst_clip_x - dst_region_x;

   xp = xpoints;// + dxx;
   yp = ypoints;// + dyy;
   xapp = xapoints;// + dxx;
   yapp = yapoints;// + dyy;
   pbuf = buf;

   if (src->cache_entry.flags.alpha)
     {
        y = 0;
	while (dst_clip_h--)
	  {
	    Cy = *yapp >> 16;
	    yap = *yapp & 0xffff;

	    while (dst_clip_w--)
	      {
		pix = *yp + *xp + pos;

		a = (A_VAL(pix) * yap) >> 10;
		r = (R_VAL(pix) * yap) >> 10;
		g = (G_VAL(pix) * yap) >> 10;
		b = (B_VAL(pix) * yap) >> 10;
		for (j = (1 << 14) - yap; j > Cy; j -= Cy)
		  {
		    pix += src_w;
		    a += (A_VAL(pix) * Cy) >> 10;
		    r += (R_VAL(pix) * Cy) >> 10;
		    g += (G_VAL(pix) * Cy) >> 10;
		    b += (B_VAL(pix) * Cy) >> 10;
		  }
		if (j > 0)
		  {
		    pix += src_w;
		    a += (A_VAL(pix) * j) >> 10;
		    r += (R_VAL(pix) * j) >> 10;
		    g += (G_VAL(pix) * j) >> 10;
		    b += (B_VAL(pix) * j) >> 10;
		  }
		if ((xap = *xapp) > 0)
		  {
		    pix = *yp + *xp + 1 + pos;
		    aa = (A_VAL(pix) * yap) >> 10;
		    rr = (R_VAL(pix) * yap) >> 10;
		    gg = (G_VAL(pix) * yap) >> 10;
		    bb = (B_VAL(pix) * yap) >> 10;
		    for (j = (1 << 14) - yap; j > Cy; j -= Cy)
		      {
			pix += src_w;
			aa += (A_VAL(pix) * Cy) >> 10;
			rr += (R_VAL(pix) * Cy) >> 10;
			gg += (G_VAL(pix) * Cy) >> 10;
			bb += (B_VAL(pix) * Cy) >> 10;
		      }
		    if (j > 0)
		      {
			pix += src_w;
			aa += (A_VAL(pix) * j) >> 10;
			rr += (R_VAL(pix) * j) >> 10;
			gg += (G_VAL(pix) * j) >> 10;
			bb += (B_VAL(pix) * j) >> 10;
		      }
		    a += ((aa - a) * xap) >> 8;
		    r += ((rr - r) * xap) >> 8;
		    g += ((gg - g) * xap) >> 8;
		    b += ((bb - b) * xap) >> 8;
		  }
		*pbuf++ = ARGB_JOIN(((a + (1 << 3)) >> 4),
				    ((r + (1 << 3)) >> 4),
				    ((g + (1 << 3)) >> 4),
				    ((b + (1 << 3)) >> 4));
		xp++;  xapp++;
	      }

            if (!mask_ie)
              func(buf, NULL, mul_col, dptr, w);
            else
              {
                 mask = mask_ie->image.data8
                    + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                    + (dst_clip_x - mask_x);

                 if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, w);
                 func(buf, mask, 0, dptr, w);
              }
            y++;

	    pbuf = buf;
	    dptr += dst_w;  dst_clip_w = w;
	    yp++;  yapp++;
	    xp = xpoints;// + dxx;
	    xapp = xapoints;// + dxx;
	  }
     }
   else
     {
#ifdef DIRECT_SCALE
        if ((!src->cache_entry.flags.alpha) &&
            (!dst->cache_entry.flags.alpha) &&
            (mul_col == 0xffffffff) &&
            (!mask_ie))
	  {
	     while (dst_clip_h--)
	       {
		 Cy = *yapp >> 16;
		 yap = *yapp & 0xffff;

		 pbuf = dptr;
		 while (dst_clip_w--)
		   {
		     pix = *yp + *xp + pos;

		     r = (R_VAL(pix) * yap) >> 10;
		     g = (G_VAL(pix) * yap) >> 10;
		     b = (B_VAL(pix) * yap) >> 10;
		     for (j = (1 << 14) - yap; j > Cy; j -= Cy)
		       {
			 pix += src_w;
			 r += (R_VAL(pix) * Cy) >> 10;
			 g += (G_VAL(pix) * Cy) >> 10;
			 b += (B_VAL(pix) * Cy) >> 10;
		       }
		     if (j > 0)
		       {
			 pix += src_w;
			 r += (R_VAL(pix) * j) >> 10;
			 g += (G_VAL(pix) * j) >> 10;
			 b += (B_VAL(pix) * j) >> 10;
		       }
		     if ((xap = *xapp) > 0)
		       {
			 pix = *yp + *xp + 1 + pos;
			 rr = (R_VAL(pix) * yap) >> 10;
			 gg = (G_VAL(pix) * yap) >> 10;
			 bb = (B_VAL(pix) * yap) >> 10;
			 for (j = (1 << 14) - yap; j > Cy; j -= Cy)
			   {
			     pix += src_w;
			     rr += (R_VAL(pix) * Cy) >> 10;
			     gg += (G_VAL(pix) * Cy) >> 10;
			     bb += (B_VAL(pix) * Cy) >> 10;
			   }
			 if (j > 0)
			   {
			     pix += src_w;
			     rr += (R_VAL(pix) * j) >> 10;
			     gg += (G_VAL(pix) * j) >> 10;
			     bb += (B_VAL(pix) * j) >> 10;
			   }
			 r += ((rr - r) * xap) >> 8;
			 g += ((gg - g) * xap) >> 8;
			 b += ((bb - b) * xap) >> 8;
		       }
		     *pbuf++ = ARGB_JOIN(0xff,
					 ((r + (1 << 3)) >> 4),
					 ((g + (1 << 3)) >> 4),
					 ((b + (1 << 3)) >> 4));
		     xp++;  xapp++;
		   }

		 dptr += dst_w;  dst_clip_w = w;
		 yp++;  yapp++;
		 xp = xpoints;// + dxx;
		 xapp = xapoints;// + dxx;
	       }
	  }
	else
#endif
	  {
             y = 0;
	     while (dst_clip_h--)
	       {
		 Cy = *yapp >> 16;
		 yap = *yapp & 0xffff;

		 while (dst_clip_w--)
		   {
		     pix = *yp + *xp + pos;

		     r = (R_VAL(pix) * yap) >> 10;
		     g = (G_VAL(pix) * yap) >> 10;
		     b = (B_VAL(pix) * yap) >> 10;
		     for (j = (1 << 14) - yap; j > Cy; j -= Cy)
		       {
			 pix += src_w;
			 r += (R_VAL(pix) * Cy) >> 10;
			 g += (G_VAL(pix) * Cy) >> 10;
			 b += (B_VAL(pix) * Cy) >> 10;
		       }
		     if (j > 0)
		       {
			 pix += src_w;
			 r += (R_VAL(pix) * j) >> 10;
			 g += (G_VAL(pix) * j) >> 10;
			 b += (B_VAL(pix) * j) >> 10;
		       }
		     if ((xap = *xapp) > 0)
		       {
			 pix = *yp + *xp + 1 + pos;
			 rr = (R_VAL(pix) * yap) >> 10;
			 gg = (G_VAL(pix) * yap) >> 10;
			 bb = (B_VAL(pix) * yap) >> 10;
			 for (j = (1 << 14) - yap; j > Cy; j -= Cy)
			   {
			     pix += src_w;
			     rr += (R_VAL(pix) * Cy) >> 10;
			     gg += (G_VAL(pix) * Cy) >> 10;
			     bb += (B_VAL(pix) * Cy) >> 10;
			   }
			 if (j > 0)
			   {
			     pix += src_w;
			     rr += (R_VAL(pix) * j) >> 10;
			     gg += (G_VAL(pix) * j) >> 10;
			     bb += (B_VAL(pix) * j) >> 10;
			   }
			 r += ((rr - r) * xap) >> 8;
			 g += ((gg - g) * xap) >> 8;
			 b += ((bb - b) * xap) >> 8;
		       }
		     *pbuf++ = ARGB_JOIN(0xff,
					 ((r + (1 << 3)) >> 4),
					 ((g + (1 << 3)) >> 4),
					 ((b + (1 << 3)) >> 4));
		     xp++;  xapp++;
		   }

                 if (!mask_ie)
                   func(buf, NULL, mul_col, dptr, w);
                 else
                   {
                      mask = mask_ie->image.data8
                         + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                         + (dst_clip_x - mask_x);

                      if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, w);
                      func(buf, mask, 0, dptr, w);
                   }
                 y++;

		 pbuf = buf;
		 dptr += dst_w;  dst_clip_w = w;
		 yp++;  yapp++;
		 xp = xpoints;// + dxx;
		 xapp = xapoints;// + dxx;
	       }
	  }
     }
}
