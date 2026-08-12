#ifndef SIGF32_RFFT_ADAPTER_H
#define SIGF32_RFFT_ADAPTER_H
#include <stddef.h>
#include "sigf32.h"
sigf32_status_t sigf32_rfft_reference(const float *input, size_t fft_size,
                                      float *real, float *imag);
#if defined(SIGF32_ENABLE_C2000WARE_RFFT)
#include "fpu32/fpu_rfft.h"
sigf32_status_t sigf32_rfft_c2000ware(RFFT_F32_STRUCT *object,
                                      float *input, float *output,
                                      float *twiddles, size_t fft_size,
                                      float *real, float *imag);
#endif
#endif
