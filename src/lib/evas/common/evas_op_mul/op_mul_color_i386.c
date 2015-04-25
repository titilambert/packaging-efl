/* mul color --> dst */

#ifdef BUILD_MMX
static void
_op_mul_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_255, mm5)
   MOV_P2R(c, mm2, mm0)
   for (; d < e; d++) {
	MOV_P2R(*d, mm1, mm0)
	MUL4_SYM_R2R(mm2, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
   }
}

#define _op_mul_can_dp_mmx _op_mul_c_dp_mmx
#define _op_mul_caa_dp_mmx _op_mul_c_dp_mmx

#define _op_mul_c_dpan_mmx _op_mul_c_dp_mmx
#define _op_mul_can_dpan_mmx _op_mul_can_dp_mmx
#define _op_mul_caa_dpan_mmx _op_mul_caa_dp_mmx

static void
init_mul_color_span_funcs_mmx(void)
{
   op_mul_span_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_mul_c_dp_mmx;
   op_mul_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_can_dp_mmx;
   op_mul_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_caa_dp_mmx;

   op_mul_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_c_dpan_mmx;
   op_mul_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_can_dpan_mmx;
   op_mul_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
static void
_op_mul_pt_c_dp_mmx(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_255, mm5)
	MOV_P2R(c, mm2, mm0)
	MOV_P2R(*d, mm1, mm0)
	MUL4_SYM_R2R(mm2, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
}

#define _op_mul_pt_caa_dp_mmx _op_mul_pt_c_dp_mmx
#define _op_mul_pt_can_dp_mmx _op_mul_pt_c_dp_mmx

#define _op_mul_pt_c_dpan_mmx _op_mul_pt_c_dp_mmx
#define _op_mul_pt_can_dpan_mmx _op_mul_pt_can_dp_mmx
#define _op_mul_pt_caa_dpan_mmx _op_mul_pt_caa_dp_mmx

static void
init_mul_color_pt_funcs_mmx(void)
{
   op_mul_pt_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_mul_pt_c_dp_mmx;
   op_mul_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_pt_can_dp_mmx;
   op_mul_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_pt_caa_dp_mmx;

   op_mul_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_pt_c_dpan_mmx;
   op_mul_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_pt_can_dpan_mmx;
   op_mul_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_pt_caa_dpan_mmx;
}
#endif
