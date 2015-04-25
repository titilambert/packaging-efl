/* blend pixel x color --> dst */

static void
_op_blend_p_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 sc = MUL4_SYM(c, *s);
                        alpha = 256 - (sc >> 24);
                        *d = sc + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

static void
_op_blend_pan_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha = 256 - (c >> 24);
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = ((c & 0xff000000) + MUL3_SYM(c, *s)) + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

static void
_op_blend_p_can_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        alpha = 256 - (*s >> 24);
                        *d = ((*s & 0xff000000) + MUL3_SYM(c, *s)) + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

static void
_op_blend_pan_can_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d++ = 0xff000000 + MUL3_SYM(c, *s);
                        s++;
                     });
}

static void
_op_blend_p_caa_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   c = 1 + (c & 0xff);
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 sc = MUL_256(c, *s);
                        alpha = 256 - (sc >> 24);
                        *d = sc + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

static void
_op_blend_pan_caa_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   c = 1 + (c & 0xff);
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = INTERP_256(c, *s, *d);
                        d++;
                        s++;
                     });
}

#define _op_blend_pas_c_dp _op_blend_p_c_dp
#define _op_blend_pas_can_dp _op_blend_p_can_dp
#define _op_blend_pas_caa_dp _op_blend_p_caa_dp

#define _op_blend_p_c_dpan _op_blend_p_c_dp
#define _op_blend_pas_c_dpan _op_blend_pas_c_dp
#define _op_blend_pan_c_dpan _op_blend_pan_c_dp
#define _op_blend_p_can_dpan _op_blend_p_can_dp
#define _op_blend_pas_can_dpan _op_blend_pas_can_dp
#define _op_blend_pan_can_dpan _op_blend_pan_can_dp
#define _op_blend_p_caa_dpan _op_blend_p_caa_dp
#define _op_blend_pas_caa_dpan _op_blend_pas_caa_dp
#define _op_blend_pan_caa_dpan _op_blend_pan_caa_dp

static void
init_blend_pixel_color_span_funcs_c(void)
{
   op_blend_span_funcs[SP][SM_N][SC][DP][CPU_C] = _op_blend_p_c_dp;
   op_blend_span_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_blend_pas_c_dp;
   op_blend_span_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_blend_pan_c_dp;
   op_blend_span_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_blend_p_can_dp;
   op_blend_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_blend_pas_can_dp;
   op_blend_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_blend_pan_can_dp;
   op_blend_span_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_blend_p_caa_dp;
   op_blend_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_blend_pas_caa_dp;
   op_blend_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_blend_pan_caa_dp;

   op_blend_span_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_blend_p_c_dpan;
   op_blend_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_blend_pas_c_dpan;
   op_blend_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_blend_pan_c_dpan;
   op_blend_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_p_can_dpan;
   op_blend_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pas_can_dpan;
   op_blend_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pan_can_dpan;
   op_blend_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_p_caa_dpan;
   op_blend_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pas_caa_dpan;
   op_blend_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pan_caa_dpan;
}

static void
_op_blend_pt_p_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = MUL4_SYM(c, s);
   c = 256 - (s >> 24);
   *d = s + MUL_256(c, *d);
}

#define _op_blend_pt_pas_c_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pan_c_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_p_can_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pas_can_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pan_can_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_p_caa_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pas_caa_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pan_caa_dp _op_blend_pt_p_c_dp

#define _op_blend_pt_p_c_dpan _op_blend_pt_p_c_dp
#define _op_blend_pt_pas_c_dpan _op_blend_pt_pas_c_dp
#define _op_blend_pt_pan_c_dpan _op_blend_pt_pan_c_dp
#define _op_blend_pt_p_can_dpan _op_blend_pt_p_can_dp
#define _op_blend_pt_pas_can_dpan _op_blend_pt_pas_can_dp
#define _op_blend_pt_pan_can_dpan _op_blend_pt_pan_can_dp
#define _op_blend_pt_p_caa_dpan _op_blend_pt_p_caa_dp
#define _op_blend_pt_pas_caa_dpan _op_blend_pt_pas_caa_dp
#define _op_blend_pt_pan_caa_dpan _op_blend_pt_pan_caa_dp

static void
init_blend_pixel_color_pt_funcs_c(void)
{
   op_blend_pt_funcs[SP][SM_N][SC][DP][CPU_C] = _op_blend_pt_p_c_dp;
   op_blend_pt_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_blend_pt_pas_c_dp;
   op_blend_pt_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_blend_pt_pan_c_dp;
   op_blend_pt_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_blend_pt_p_can_dp;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_blend_pt_pas_can_dp;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_blend_pt_pan_can_dp;
   op_blend_pt_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_blend_pt_p_caa_dp;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_blend_pt_pas_caa_dp;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_blend_pt_pan_caa_dp;

   op_blend_pt_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_blend_pt_p_c_dpan;
   op_blend_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_blend_pt_pas_c_dpan;
   op_blend_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_blend_pt_pan_c_dpan;
   op_blend_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pt_p_can_dpan;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pt_pas_can_dpan;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pt_pan_can_dpan;
   op_blend_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pt_p_caa_dpan;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pt_pas_caa_dpan;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pt_pan_caa_dpan;
}

/*-----*/

/* blend_rel pixel x color -> dst */

static void
_op_blend_rel_p_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 sc = MUL4_SYM(c, *s);
                        alpha = 256 - (sc >> 24);
                        *d = MUL_SYM(*d >> 24, sc) + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

#define _op_blend_rel_pas_c_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pan_c_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_p_can_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pas_can_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pan_can_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_p_caa_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pas_caa_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pan_caa_dp _op_blend_rel_p_c_dp

#define _op_blend_rel_p_c_dpan _op_blend_p_c_dpan
#define _op_blend_rel_pas_c_dpan _op_blend_pas_c_dpan
#define _op_blend_rel_pan_c_dpan _op_blend_pan_c_dpan
#define _op_blend_rel_p_can_dpan _op_blend_p_can_dpan
#define _op_blend_rel_pas_can_dpan _op_blend_pas_can_dpan
#define _op_blend_rel_pan_can_dpan _op_blend_pan_can_dpan
#define _op_blend_rel_p_caa_dpan _op_blend_p_caa_dpan
#define _op_blend_rel_pas_caa_dpan _op_blend_pas_caa_dpan
#define _op_blend_rel_pan_caa_dpan _op_blend_pan_caa_dpan

static void
init_blend_rel_pixel_color_span_funcs_c(void)
{
   op_blend_rel_span_funcs[SP][SM_N][SC][DP][CPU_C] = _op_blend_rel_p_c_dp;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_blend_rel_pas_c_dp;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_blend_rel_pan_c_dp;
   op_blend_rel_span_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_p_can_dp;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pas_can_dp;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pan_can_dp;
   op_blend_rel_span_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_p_caa_dp;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pas_caa_dp;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pan_caa_dp;

   op_blend_rel_span_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_p_c_dpan;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pas_c_dpan;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pan_c_dpan;
   op_blend_rel_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_p_can_dpan;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pas_can_dpan;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pan_can_dpan;
   op_blend_rel_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_p_caa_dpan;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pas_caa_dpan;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pan_caa_dpan;
}

static void
_op_blend_rel_pt_p_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = MUL4_SYM(c, s);
   c = 256 - (s >> 24);
   *d = MUL_SYM(*d >> 24, s) + MUL_256(c, *d);
}

#define _op_blend_rel_pt_pas_c_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pan_c_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_p_can_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pas_can_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pan_can_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_p_caa_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pas_caa_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pan_caa_dp _op_blend_rel_pt_p_c_dp

#define _op_blend_rel_pt_p_c_dpan _op_blend_pt_p_c_dpan
#define _op_blend_rel_pt_pas_c_dpan _op_blend_pt_pas_c_dpan
#define _op_blend_rel_pt_pan_c_dpan _op_blend_pt_pan_c_dpan
#define _op_blend_rel_pt_p_can_dpan _op_blend_pt_p_can_dpan
#define _op_blend_rel_pt_pas_can_dpan _op_blend_pt_pas_can_dpan
#define _op_blend_rel_pt_pan_can_dpan _op_blend_pt_pan_can_dpan
#define _op_blend_rel_pt_p_caa_dpan _op_blend_pt_p_caa_dpan
#define _op_blend_rel_pt_pas_caa_dpan _op_blend_pt_pas_caa_dpan
#define _op_blend_rel_pt_pan_caa_dpan _op_blend_pt_pan_caa_dpan

static void
init_blend_rel_pixel_color_pt_funcs_c(void)
{
   op_blend_rel_pt_funcs[SP][SM_N][SC][DP][CPU_C] = _op_blend_rel_pt_p_c_dp;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_blend_rel_pt_pas_c_dp;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_blend_rel_pt_pan_c_dp;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pt_p_can_dp;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pt_pas_can_dp;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pt_pan_can_dp;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pt_p_caa_dp;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pt_pas_caa_dp;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pt_pan_caa_dp;

   op_blend_rel_pt_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pt_p_c_dpan;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pt_pas_c_dpan;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pt_pan_c_dpan;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pt_p_can_dpan;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pt_pas_can_dpan;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pt_pan_can_dpan;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pt_p_caa_dpan;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pt_pas_caa_dpan;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pt_pan_caa_dpan;
}
