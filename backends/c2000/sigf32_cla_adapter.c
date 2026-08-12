#include "sigf32_cla_adapter.h"

/* Host-safe bridge. For a CLA build, compile this file with the CLA toolchain,
 * place input/output/state in CPU-to-CLA message RAM, and trigger a CLA task. */
float sigf32_cla_fir_point(sigf32_fir_t *fir, float input) { return sigf32_fir_process(fir, input); }
float sigf32_cla_biquad_point(sigf32_biquad_t *filter, float input) { return sigf32_biquad_process(filter, input); }
sigf32_iq_result_t sigf32_cla_iq_point(sigf32_iq_demod_t *demod, float input) { return sigf32_iq_process(demod, input); }
float sigf32_cla_pll_point(sigf32_pll_t *pll, float input) { return sigf32_pll_process(pll, input); }
float sigf32_cla_lms_point(sigf32_lms_t *lms, float reference, float desired, float *error)
{ return sigf32_lms_process(lms, reference, desired, error); }
