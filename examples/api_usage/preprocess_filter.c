#include "sigf32.h"

int main(void)
{
    enum { BLOCK = 16, TAPS = 3 };
    static const float fir_coeffs[TAPS] = {0.25f, 0.5f, 0.25f};
    static const sigf32_biquad_coeffs_t sos[1] = {{0.5f, 0.0f, 0.0f, -0.5f, 0.0f}};
    static float fir_state[TAPS];
    static float decimator_state[TAPS];
    static float input[BLOCK];
    static float filtered[BLOCK];
    static sigf32_biquad_state_t sos_state[1];
    sigf32_dc_blocker_t dc;
    sigf32_fir_t fir;
    sigf32_biquad_t biquad;
    sigf32_decimator_t decimator;
    sigf32_diag_t diag = {0};
    float decimated = 0.0f;

    if (sigf32_dc_blocker_init(&dc, 0.995f) != SIGF32_OK ||
        sigf32_fir_init(&fir, fir_coeffs, TAPS, fir_state, TAPS) != SIGF32_OK ||
        sigf32_biquad_init(&biquad, sos, 1U, sos_state, 1U) != SIGF32_OK ||
        sigf32_decimator_init(&decimator, fir_coeffs, TAPS, decimator_state, TAPS, 2U) != SIGF32_OK)
        return 1;
    for (unsigned i = 0U; i < BLOCK; ++i)
        input[i] = sigf32_dc_blocker_process(&dc, sigf32_linear_calibrate((float)i, 0.001f, -0.01f), &diag);
    if (sigf32_fir_process_block(&fir, input, filtered, BLOCK, &diag) != SIGF32_OK)
        return 2;
    for (unsigned i = 0U; i < BLOCK; ++i) {
        float y = sigf32_biquad_process(&biquad, filtered[i]);
        if (sigf32_decimator_process(&decimator, y, &decimated) != 0U)
            filtered[i] = decimated;
    }
    sigf32_dc_blocker_reset(&dc);
    sigf32_fir_reset(&fir);
    sigf32_biquad_reset(&biquad);
    sigf32_decimator_reset(&decimator);
    return diag.invalid_count != 0U;
}
