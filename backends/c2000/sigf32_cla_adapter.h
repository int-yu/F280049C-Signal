#ifndef SIGF32_CLA_ADAPTER_H
#define SIGF32_CLA_ADAPTER_H

#include "sigf32.h"

/* These point operations are deliberately message-RAM friendly: caller owns all state. */
float sigf32_cla_fir_point(sigf32_fir_t *fir, float input);
float sigf32_cla_biquad_point(sigf32_biquad_t *filter, float input);
sigf32_iq_result_t sigf32_cla_iq_point(sigf32_iq_demod_t *demod, float input);
float sigf32_cla_pll_point(sigf32_pll_t *pll, float input);
float sigf32_cla_lms_point(sigf32_lms_t *lms, float reference, float desired, float *error);

#endif
