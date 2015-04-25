/* blend pixel --> dst */

#ifdef BUILD_MMX
static void
_op_blend_p_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   while (d < e)
     {
	MOV_P2R(*s, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	movq_r2r(mm6, mm3);
	psubw_r2r(mm1, mm3);

	MOV_P2R(*d, mm1, mm0)
	MUL4_256_R2R(mm3, mm1)

	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
	s++;  d++;
     }
}

static void
_op_blend_pas_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   _op_blend_p_dp_mmx(s, m, c, d, l);
   return;
/*   
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   while (d < e)
     {
	switch (*s & 0xff000000)
	  {
	    case 0:
	      break;
	    case 0xff000000:
		*d = *s;
	      break;
	    default :
		MOV_P2R(*s, mm2, mm0)
		MOV_RA2R(mm2, mm1)
		movq_r2r(mm6, mm3);
		psubw_r2r(mm1, mm3);

		MOV_P2R(*d, mm1, mm0)
		MUL4_256_R2R(mm3, mm1)

		paddw_r2r(mm2, mm1);
		MOV_R2P(mm1, *d, mm0)
	      break;
	  }
	s++;  d++;
     }
 */
}

#define _op_blend_pan_dp_mmx NULL

#define _op_blend_p_dpan_mmx _op_blend_p_dp_mmx
#define _op_blend_pas_dpan_mmx _op_blend_pas_dp_mmx
#define _op_blend_pan_dpan_mmx _op_blend_pan_dp_mmx

static void
init_blend_pixel_span_funcs_mmx(void)
{
   op_blend_span_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_blend_p_dp_mmx;
   op_blend_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pas_dp_mmx;
   op_blend_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pan_dp_mmx;

   op_blend_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_p_dpan_mmx;
   op_blend_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pas_dpan_mmx;
   op_blend_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pan_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
static void
_op_blend_pt_p_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_256, mm6)
	MOV_P2R(s, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	movq_r2r(mm6, mm3);
	psubw_r2r(mm1, mm3);

	MOV_P2R(*d, mm1, mm0)
	MUL4_256_R2R(mm3, mm1)

	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
}


#define _op_blend_pt_pan_dp_mmx NULL
#define _op_blend_pt_pas_dp_mmx _op_blend_pt_p_dp_mmx

#define _op_blend_pt_p_dpan_mmx _op_blend_pt_p_dp_mmx
#define _op_blend_pt_pan_dpan_mmx _op_blend_pt_pan_dp_mmx
#define _op_blend_pt_pas_dpan_mmx _op_blend_pt_pas_dp_mmx

static void
init_blend_pixel_pt_funcs_mmx(void)
{
   op_blend_pt_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pt_p_dp_mmx;
   op_blend_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pt_pas_dp_mmx;
   op_blend_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pt_pan_dp_mmx;

   op_blend_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_p_dpan_mmx;
   op_blend_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_pas_dpan_mmx;
   op_blend_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_pan_dpan_mmx;
}
#endif

/*-----*/

/* blend_rel pixel -> dst */

#ifdef BUILD_MMX
static void
_op_blend_rel_p_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   MOV_A2R(ALPHA_255, mm5)
   while (d < e)
     {
	MOV_P2R(*s, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	movq_r2r(mm6, mm3);
	psubw_r2r(mm1, mm3);

	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm4)
	MUL4_256_R2R(mm3, mm1)

	MUL4_SYM_R2R(mm4, mm2, mm5)
	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
	s++;  d++;
     }
}

static void
_op_blend_rel_pan_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   MOV_A2R(ALPHA_255, mm5)
   while (d < e)
     {
	MOV_P2R(*s, mm2, mm0)
	MOV_PA2R(*d, mm1)
	MUL4_SYM_R2R(mm2, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
	s++;  d++;
     }
}

#define _op_blend_rel_pas_dp_mmx _op_blend_rel_p_dp_mmx

#define _op_blend_rel_p_dpan_mmx _op_blend_p_dpan_mmx
#define _op_blend_rel_pan_dpan_mmx _op_blend_pan_dpan_mmx
#define _op_blend_rel_pas_dpan_mmx _op_blend_pas_dpan_mmx

static void
init_blend_rel_pixel_span_funcs_mmx(void)
{
   op_blend_rel_span_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_p_dp_mmx;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pas_dp_mmx;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pan_dp_mmx;

   op_blend_rel_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_p_dpan_mmx;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pas_dpan_mmx;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pan_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
static void
_op_blend_rel_pt_p_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_256, mm6)
	MOV_A2R(ALPHA_255, mm5)

	MOV_P2R(s, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	psubw_r2r(mm1, mm6);

	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm4)
	MUL4_256_R2R(mm6, mm1)

	MUL4_SYM_R2R(mm4, mm2, mm5)
	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
}

#define _op_blend_rel_pt_pas_dp_mmx _op_blend_rel_pt_p_dp_mmx
#define _op_blend_rel_pt_pan_dp_mmx _op_blend_rel_pt_p_dp_mmx

#define _op_blend_rel_pt_p_dpan_mmx _op_blend_pt_p_dpan_mmx
#define _op_blend_rel_pt_pas_dpan_mmx _op_blend_pt_pas_dpan_mmx
#define _op_blend_rel_pt_pan_dpan_mmx _op_blend_pt_pan_dpan_mmx

static void
init_blend_rel_pixel_pt_funcs_mmx(void)
{
   op_blend_rel_pt_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pt_p_dp_mmx;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pt_pas_dp_mmx;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pt_pan_dp_mmx;

   op_blend_rel_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_p_dpan_mmx;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_pas_dpan_mmx;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_pan_dpan_mmx;
}
#endif
