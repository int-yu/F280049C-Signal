#include <math.h>
#include "sigf32.h"

int main(void)
{
    enum { N = 128 };
    static float x[N], y[N], windowed[N];
    static float workspace[2 * (N / 2 + 1)];
    static const float targets[2] = {50.0f, 100.0f};
    sigf32_stats_result_t stats;
    sigf32_frequency_result_t frequency;
    sigf32_goertzel_result_t tones[2];
    sigf32_spectrum_metrics_t spectrum;
    sigf32_correlation_result_t delay;
    for (unsigned i = 0U; i < N; ++i) {
        x[i] = 0.8f * sinf(SIGF32_TWO_PI * 50.0f * (float)i / 1024.0f);
        y[i] = i == 0U ? 0.0f : x[i - 1U];
    }
    sigf32_apply_window(x, windowed, N, SIGF32_WINDOW_HANN);
    if (sigf32_stats(x, N, &stats) != SIGF32_OK ||
        sigf32_frequency_zero_cross(x, N, 1024.0f, 0.01f, &frequency) != SIGF32_OK ||
        sigf32_goertzel_multi(x, N, 1024.0f, targets, tones, 2U) != SIGF32_OK ||
        sigf32_spectrum_analyze(x, N, 1024.0f, SIGF32_WINDOW_HANN, 4U, workspace,
                                2U * (N / 2U + 1U), &spectrum) != SIGF32_OK ||
        sigf32_cross_correlate(x, y, N, 4, &delay) != SIGF32_OK)
        return 1;
    return (stats.valid == 0U || frequency.valid == 0U || tones[0].valid == 0U ||
            spectrum.valid == 0U || delay.valid == 0U) ? 2 : 0;
}
