#include <math.h>
#include <stdio.h>

#include "sigf32.h"

#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); return 1; \
} } while (0)

int main(void)
{
    float ma_state[4] = {0};
    sigf32_moving_average_t ma;
    CHECK(sigf32_moving_average_init(&ma, ma_state, 4, 4) == SIGF32_OK);
    CHECK(fabsf(sigf32_moving_average_process(&ma, 1.0f) - 0.25f) < 1e-6f);
    CHECK(fabsf(sigf32_moving_average_process(&ma, 1.0f) - 0.50f) < 1e-6f);
    CHECK(fabsf(sigf32_moving_average_process(&ma, 1.0f) - 0.75f) < 1e-6f);
    CHECK(fabsf(sigf32_moving_average_process(&ma, 1.0f) - 1.00f) < 1e-6f);

    float fir_coeffs[1] = {1.0f};
    float fir_state[1] = {0};
    sigf32_decimator_t decimator;
    CHECK(sigf32_decimator_init(&decimator, fir_coeffs, 1, fir_state, 1, 4) == SIGF32_OK);
    unsigned outputs = 0;
    for (unsigned i = 0; i < 12; ++i) {
        float output = 0.0f;
        if (sigf32_decimator_process(&decimator, (float)i, &output)) {
            CHECK(output == (float)i);
            outputs++;
        }
    }
    CHECK(outputs == 3);

    sigf32_agc_t agc;
    CHECK(sigf32_agc_init(&agc, 0.5f, 0.05f, 0.01f, 0.1f, 20.0f) == SIGF32_OK);
    float y = 0.0f;
    for (unsigned i = 0; i < 4000; ++i) {
        y = sigf32_agc_process(&agc, 0.1f * sinf(SIGF32_TWO_PI * i / 40.0f), NULL);
    }
    CHECK(isfinite(y));
    CHECK(agc.gain > 2.0f && agc.gain < 10.0f);

    float block[1000];
    for (unsigned i = 0; i < 1000; ++i) {
        block[i] = sinf(SIGF32_TWO_PI * 53.0f * i / 1000.0f);
    }
    sigf32_fll_t fll;
    CHECK(sigf32_fll_init(&fll, 1000.0f, 45.0f, 0.5f, 20.0f, 80.0f) == SIGF32_OK);
    for (unsigned i = 0; i < 8; ++i) {
        CHECK(sigf32_fll_update_block(&fll, block, 1000, 0.05f) == SIGF32_OK);
    }
    CHECK(fabsf(fll.frequency_hz - 53.0f) < 0.1f);
    CHECK(fll.locked);

    puts("F280049C streaming tests: PASS");
    return 0;
}
