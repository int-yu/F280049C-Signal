#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sigf32.h"

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); return 1; \
} } while (0)

static int nearf32(float a, float b, float tol)
{
    return fabsf(a - b) <= tol;
}

static int test_numeric_and_preprocess(void)
{
    sigf32_dc_blocker_t dc;
    sigf32_diag_t diag = {0};
    CHECK(nearf32(sigf32_phase_wrap_pu(1.25f), 0.25f, 1.0e-6f));
    CHECK(sigf32_dc_blocker_init(&dc, 0.995f) == SIGF32_OK);
    for (int i = 0; i < 4000; ++i) {
        (void)sigf32_dc_blocker_process(&dc, 0.75f, &diag);
    }
    CHECK(fabsf(sigf32_dc_blocker_process(&dc, 0.75f, &diag)) < 1.0e-3f);
    CHECK(nearf32(sigf32_median3(9.0f, -1.0f, 2.0f), 2.0f, 0.0f));
    return 0;
}

static int test_filter_and_measure(void)
{
    float coeffs[3] = {0.25f, 0.5f, 0.25f};
    float state[3] = {0};
    sigf32_fir_t fir;
    sigf32_diag_t diag = {0};
    float impulse[5] = {1, 0, 0, 0, 0};
    float output[5] = {0};
    CHECK(sigf32_fir_init(&fir, coeffs, 3, state, 3) == SIGF32_OK);
    CHECK(sigf32_fir_process_block(&fir, impulse, output, 5, &diag) == SIGF32_OK);
    CHECK(nearf32(output[0], 0.25f, 1.0e-6f));
    CHECK(nearf32(output[1], 0.5f, 1.0e-6f));
    CHECK(nearf32(output[2], 0.25f, 1.0e-6f));

    float x[1000];
    for (int i = 0; i < 1000; ++i) x[i] = 0.8f * sinf(2.0f * SIGF32_PI * 50.0f * i / 1000.0f);
    sigf32_stats_result_t stats;
    CHECK(sigf32_stats(x, 1000, &stats) == SIGF32_OK);
    CHECK(nearf32(stats.rms, 0.8f / sqrtf(2.0f), 1.0e-4f));
    sigf32_frequency_result_t freq;
    CHECK(sigf32_frequency_zero_cross(x, 1000, 1000.0f, 0.05f, &freq) == SIGF32_OK);
    CHECK(freq.valid && nearf32(freq.frequency_hz, 50.0f, 0.03f));
    return 0;
}

static int test_spectral_and_nco(void)
{
    float x[256];
    for (int i = 0; i < 256; ++i) x[i] = 0.6f * sinf(2.0f * SIGF32_PI * 32.0f * i / 256.0f);
    sigf32_goertzel_result_t g;
    CHECK(sigf32_goertzel(x, 256, 256.0f, 32.0f, &g) == SIGF32_OK);
    CHECK(g.valid && nearf32(g.amplitude_peak, 0.6f, 2.0e-3f));

    sigf32_nco_t nco;
    CHECK(sigf32_nco_init(&nco, 1000.0f, 125.0f, 0.0f) == SIGF32_OK);
    float s0, c0, s1, c1;
    sigf32_nco_next(&nco, &s0, &c0);
    sigf32_nco_next(&nco, &s1, &c1);
    CHECK(nearf32(s0, 0.0f, 1.0e-6f) && nearf32(c0, 1.0f, 1.0e-6f));
    CHECK(nearf32(s1, 0.70710678f, 1.0e-5f));
    return 0;
}

static int test_tracking(void)
{
    float weights[8] = {0};
    float history[8] = {0};
    sigf32_lms_t lms;
    CHECK(sigf32_lms_init(&lms, weights, history, 8, 0.08f, 1) == SIGF32_OK);
    float error = 0.0f;
    for (int n = 0; n < 2000; ++n) {
        float x = sinf(0.071f * n);
        float d = 0.5f * x;
        (void)sigf32_lms_process(&lms, x, d, &error);
    }
    CHECK(fabsf(error) < 0.02f);

    sigf32_pll_t pll;
    CHECK(sigf32_pll_init(&pll, 1000.0f, 48.0f, 0.02f, 0.0004f, 20.0f, 80.0f) == SIGF32_OK);
    for (int n = 0; n < 5000; ++n) {
        float sample = sinf(2.0f * SIGF32_PI * 50.0f * n / 1000.0f);
        (void)sigf32_pll_process(&pll, sample);
    }
    CHECK(fabsf(sigf32_pll_frequency_hz(&pll) - 50.0f) < 0.2f);
    for (int n = 0; n < 4000; ++n) {
        float sample = sinf(2.0f * SIGF32_PI * 55.0f * n / 1000.0f);
        (void)sigf32_pll_process(&pll, sample);
    }
    CHECK(fabsf(sigf32_pll_frequency_hz(&pll) - 55.0f) < 0.1f);
    CHECK(sigf32_pll_locked(&pll));
    for (int n = 0; n < 1000; ++n) (void)sigf32_pll_process(&pll, 0.0f);
    for (int n = 0; n < 3000; ++n) {
        float sample = sinf(2.0f * SIGF32_PI * 50.0f * n / 1000.0f);
        (void)sigf32_pll_process(&pll, sample);
    }
    CHECK(fabsf(sigf32_pll_frequency_hz(&pll) - 50.0f) < 0.1f);
    CHECK(sigf32_pll_locked(&pll));
    return 0;
}

int main(void)
{
    CHECK(test_numeric_and_preprocess() == 0);
    CHECK(test_filter_and_measure() == 0);
    CHECK(test_spectral_and_nco() == 0);
    CHECK(test_tracking() == 0);
    puts("F280049C host tests: PASS");
    return 0;
}
