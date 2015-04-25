/* copy mask x color -> dst */

static void
_op_copy_mas_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        /* d = m*c */
                        alpha = *m;
                        switch(alpha)
                          {
                          case 0:
                             *d = 0;
                             break;
                          case 255:
                             *d = c;
                             break;
                          default:
                             alpha++;
                             *d = MUL_256(alpha, c);
                             break;
                          }
                        m++;  d++;
                     });
}

#define _op_copy_mas_cn_dp _op_copy_mas_c_dp
#define _op_copy_mas_can_dp _op_copy_mas_c_dp
#define _op_copy_mas_caa_dp _op_copy_mas_c_dp

#define _op_copy_mas_c_dpan _op_copy_mas_c_dp
#define _op_copy_mas_cn_dpan _op_copy_mas_c_dpan
#define _op_copy_mas_can_dpan _op_copy_mas_c_dpan
#define _op_copy_mas_caa_dpan _op_copy_mas_c_dpan

static void
init_copy_mask_color_span_funcs_c(void)
{
   op_copy_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_copy_mas_cn_dp;
   op_copy_span_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_copy_mas_c_dp;
   op_copy_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_copy_mas_can_dp;
   op_copy_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_copy_mas_caa_dp;

   op_copy_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_mas_cn_dpan;
   op_copy_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_copy_mas_c_dpan;
   op_copy_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_copy_mas_can_dpan;
   op_copy_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_copy_mas_caa_dpan;
}

static void
_op_copy_pt_mas_c_dp(DATA32 s EINA_UNUSED, DATA8 m, DATA32 c, DATA32 *d) {
   *d = INTERP_256(m + 1, c, *d);
}


#define _op_copy_pt_mas_cn_dp _op_copy_pt_mas_c_dp
#define _op_copy_pt_mas_can_dp _op_copy_pt_mas_c_dp
#define _op_copy_pt_mas_caa_dp _op_copy_pt_mas_c_dp

#define _op_copy_pt_mas_c_dpan _op_copy_pt_mas_c_dp
#define _op_copy_pt_mas_cn_dpan _op_copy_pt_mas_c_dpan
#define _op_copy_pt_mas_can_dpan _op_copy_pt_mas_c_dpan
#define _op_copy_pt_mas_caa_dpan _op_copy_pt_mas_c_dpan

static void
init_copy_mask_color_pt_funcs_c(void)
{
   op_copy_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_copy_pt_mas_cn_dp;
   op_copy_pt_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_copy_pt_mas_c_dp;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_copy_pt_mas_can_dp;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_copy_pt_mas_caa_dp;

   op_copy_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_pt_mas_cn_dpan;
   op_copy_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_copy_pt_mas_c_dpan;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_copy_pt_mas_can_dpan;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_copy_pt_mas_caa_dpan;
}

/*-----*/

/* copy_rel mask x color -> dst */

static void
_op_copy_rel_mas_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   /* FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED */
   DATA32 *e;
   int color;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        /* d = (m*c)*da */
                        color = *m;
                        switch(color)
                          {
                          case 0:
                             *d = 0;
                             break;
                          case 255:
                             color = 1 + (*d >> 24);
                             *d = MUL_256(color, c);
                             break;
                          default:
                               {
                                  DATA32 da = 1 + (*d >> 24);
                                  da = MUL_256(da, c);
                                  color++;
                                  *d = INTERP_256(color, da, *d);
                               }
                             break;
                          }
                        m++;  d++;
                     });
}


#define _op_copy_rel_mas_cn_dp _op_copy_rel_mas_c_dp
#define _op_copy_rel_mas_can_dp _op_copy_rel_mas_c_dp
#define _op_copy_rel_mas_caa_dp _op_copy_rel_mas_c_dp

#define _op_copy_rel_mas_c_dpan _op_copy_mas_c_dpan
#define _op_copy_rel_mas_cn_dpan _op_copy_mas_cn_dpan
#define _op_copy_rel_mas_can_dpan _op_copy_mas_can_dpan
#define _op_copy_rel_mas_caa_dpan _op_copy_mas_caa_dpan

static void
init_copy_rel_mask_color_span_funcs_c(void)
{
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_mas_cn_dp;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_copy_rel_mas_c_dp;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_copy_rel_mas_can_dp;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_copy_rel_mas_caa_dp;

   op_copy_rel_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_mas_cn_dpan;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_copy_rel_mas_c_dpan;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_copy_rel_mas_can_dpan;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_copy_rel_mas_caa_dpan;
}

static void
_op_copy_rel_pt_mas_c_dp(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   /* FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED */
   s = 1 + (*d >> 24);
   s = MUL_256(s, c);
   *d = INTERP_256(m + 1, s, *d);
}

#define _op_copy_rel_pt_mas_cn_dp _op_copy_rel_pt_mas_c_dp
#define _op_copy_rel_pt_mas_can_dp _op_copy_rel_pt_mas_c_dp
#define _op_copy_rel_pt_mas_caa_dp _op_copy_rel_pt_mas_c_dp

#define _op_copy_rel_pt_mas_c_dpan _op_copy_pt_mas_c_dpan
#define _op_copy_rel_pt_mas_cn_dpan _op_copy_pt_mas_cn_dpan
#define _op_copy_rel_pt_mas_can_dpan _op_copy_pt_mas_can_dpan
#define _op_copy_rel_pt_mas_caa_dpan _op_copy_pt_mas_caa_dpan

static void
init_copy_rel_mask_color_pt_funcs_c(void)
{
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_pt_mas_cn_dp;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_copy_rel_pt_mas_c_dp;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_copy_rel_pt_mas_can_dp;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_copy_rel_pt_mas_caa_dp;

   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_mas_cn_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_copy_rel_pt_mas_c_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_copy_rel_pt_mas_can_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_copy_rel_pt_mas_caa_dpan;
}
