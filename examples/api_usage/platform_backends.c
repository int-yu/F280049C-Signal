#include "sigf32.h"
#include "sigf32_cla_adapter.h"
#include "sigf32_rfft_adapter.h"
#include "sigf32_tmu.h"

int main(void)
{
    enum { N = 32, TAPS = 2 };
    static const float coeffs[TAPS] = {0.5f, 0.5f};
    static float samples[N], real[N / 2 + 1], imag[N / 2 + 1], fir_state[TAPS];
    sigf32_fir_t fir;
    float sine, cosine;
    if (sigf32_fir_init(&fir, coeffs, TAPS, fir_state, TAPS) != SIGF32_OK)
        return 1;
    sigf32_tmu_sin_cos_pu(0.125f, &sine, &cosine);
    (void)sine;
    (void)cosine;
    (void)sigf32_cla_fir_point(&fir, samples[0]);
    return sigf32_rfft_reference(samples, N, real, imag) == SIGF32_OK ? 0 : 2;
}
