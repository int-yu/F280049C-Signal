#include <math.h>
#include <stdio.h>
#include "sigf32.h"

/* Host example: gcc -std=c11 -Iinclude examples/offline_measurement.c src/sigf32.c -lm */
int main(void)
{
    enum { N = 1024 };
    float samples[N];
    /* C does not permit a function-call VLA initializer on every compiler. */
    float workspace[2 * (N / 2 + 1)];
    sigf32_stats_result_t stats;
    sigf32_frequency_result_t frequency;
    sigf32_spectrum_metrics_t spectrum;

    for (unsigned i = 0; i < N; ++i)
        samples[i] = 0.8f * sinf(SIGF32_TWO_PI * 50.0f * (float)i / 4096.0f);

    sigf32_stats(samples, N, &stats);
    sigf32_frequency_zero_cross(samples, N, 4096.0f, 0.02f, &frequency);
    sigf32_spectrum_analyze(samples, N, 4096.0f, SIGF32_WINDOW_HANN, 8,
                            workspace, 2 * (N / 2 + 1), &spectrum);
    printf("RMS=%.5f, f=%.3f Hz, THD=%.5f\n", stats.rms,
           frequency.frequency_hz, spectrum.thd);
    return 0;
}