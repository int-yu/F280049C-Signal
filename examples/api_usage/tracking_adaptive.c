#include <math.h>
#include "sigf32.h"

int main(void)
{
    enum { N = 256, TAPS = 4 };
    static float signal[N], weights[TAPS], history[TAPS];
    sigf32_nco_t nco;
    sigf32_iq_demod_t iq;
    sigf32_fll_t fll;
    sigf32_pll_t pll;
    sigf32_lms_t nlms;
    float sine, cosine, error = 0.0f;
    if (sigf32_nco_init(&nco, 1000.0f, 50.0f, 0.25f) != SIGF32_OK ||
        sigf32_iq_init(&iq, 1000.0f, 50.0f, 0.05f) != SIGF32_OK ||
        sigf32_fll_init(&fll, 1000.0f, 50.0f, 0.25f, 40.0f, 60.0f) != SIGF32_OK ||
        sigf32_lms_init(&nlms, weights, history, TAPS, 0.2f, 1U) != SIGF32_OK)
        return 1;
    for (unsigned i = 0U; i < N; ++i)
        signal[i] = sinf(SIGF32_TWO_PI * 50.0f * (float)i / 1000.0f);
    if (sigf32_fll_update_block(&fll, signal, N, 0.01f) != SIGF32_OK ||
        sigf32_pll_init(&pll, 1000.0f, fll.frequency_hz, 0.01f, 0.0001f, 40.0f, 60.0f) != SIGF32_OK)
        return 2;
    (void)sigf32_nco_set_frequency(&nco, fll.frequency_hz);
    for (unsigned i = 0U; i < N; ++i) {
        sigf32_iq_result_t result;
        sigf32_nco_next(&nco, &sine, &cosine);
        result = sigf32_iq_process(&iq, signal[i]);
        (void)result;
        (void)sigf32_pll_process(&pll, signal[i]);
        (void)sigf32_lms_process(&nlms, signal[i], 0.5f * signal[i], &error);
    }
    sigf32_lms_reset(&nlms);
    if (sigf32_pll_locked(&pll) != 0U)
        (void)sigf32_pll_frequency_hz(&pll);
    return sigf32_pll_frequency_hz(&pll) <= 0.0f;
}
