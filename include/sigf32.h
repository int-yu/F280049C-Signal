#ifndef SIGF32_H
#define SIGF32_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIGF32_PI 3.14159265358979323846f
#define SIGF32_TWO_PI (2.0f * SIGF32_PI)
#define SIGF32_VERSION_MAJOR 1
#define SIGF32_VERSION_MINOR 0
#define SIGF32_VERSION_PATCH 0

typedef enum {
    SIGF32_OK = 0,
    SIGF32_EINVAL,
    SIGF32_EWORKSPACE,
    SIGF32_ENOSIGNAL,
    SIGF32_ENOTLOCKED,
    SIGF32_ENOTCONVERGED,
    SIGF32_ERANGE
} sigf32_status_t;

typedef enum {
    SIGF32_BACKEND_PORTABLE = 0,
    SIGF32_BACKEND_C28X_TMU,
    SIGF32_BACKEND_CLA
} sigf32_backend_t;

typedef struct {
    uint32_t saturation_count;
    uint32_t invalid_count;
    uint16_t valid;
    uint16_t locked;
    uint16_t converged;
} sigf32_diag_t;

float sigf32_phase_wrap_pu(float phase_pu);
float sigf32_phase_pu_to_rad(float phase_pu);
float sigf32_phase_pu_to_deg(float phase_pu);
float sigf32_linear_calibrate(float sample, float gain, float offset);
float sigf32_median3(float a, float b, float c);

typedef struct { float alpha, x1, y1; } sigf32_dc_blocker_t;
sigf32_status_t sigf32_dc_blocker_init(sigf32_dc_blocker_t *state, float alpha);
void sigf32_dc_blocker_reset(sigf32_dc_blocker_t *state);
float sigf32_dc_blocker_process(sigf32_dc_blocker_t *state, float input, sigf32_diag_t *diag);

typedef struct { float *state; size_t length, index; float sum; } sigf32_moving_average_t;
size_t sigf32_moving_average_workspace_size(size_t length);
sigf32_status_t sigf32_moving_average_init(sigf32_moving_average_t *filter, float *state,
                                           size_t state_count, size_t length);
void sigf32_moving_average_reset(sigf32_moving_average_t *filter);
float sigf32_moving_average_process(sigf32_moving_average_t *filter, float input);

typedef struct { float target_peak, attack, release, min_gain, max_gain, gain, envelope; } sigf32_agc_t;
sigf32_status_t sigf32_agc_init(sigf32_agc_t *agc, float target_peak, float attack,
                                float release, float min_gain, float max_gain);
void sigf32_agc_reset(sigf32_agc_t *agc);
float sigf32_agc_process(sigf32_agc_t *agc, float input, sigf32_diag_t *diag);

typedef struct {
    const float *coeffs;
    float *state;
    size_t taps;
    size_t index;
} sigf32_fir_t;
size_t sigf32_fir_workspace_size(size_t taps);
sigf32_status_t sigf32_fir_init(sigf32_fir_t *fir, const float *coeffs, size_t taps,
                                float *state, size_t state_count);
void sigf32_fir_reset(sigf32_fir_t *fir);
float sigf32_fir_process(sigf32_fir_t *fir, float input);
sigf32_status_t sigf32_fir_process_block(sigf32_fir_t *fir, const float *input,
                                         float *output, size_t count, sigf32_diag_t *diag);

typedef struct { sigf32_fir_t fir; size_t factor, phase; } sigf32_decimator_t;
sigf32_status_t sigf32_decimator_init(sigf32_decimator_t *decimator,
                                      const float *coeffs, size_t taps,
                                      float *state, size_t state_count, size_t factor);
void sigf32_decimator_reset(sigf32_decimator_t *decimator);
uint16_t sigf32_decimator_process(sigf32_decimator_t *decimator, float input,
                                 float *output);

typedef struct { float b0, b1, b2, a1, a2; } sigf32_biquad_coeffs_t;
typedef struct { float d1, d2; } sigf32_biquad_state_t;
typedef struct {
    const sigf32_biquad_coeffs_t *coeffs;
    sigf32_biquad_state_t *state;
    size_t stages;
} sigf32_biquad_t;
size_t sigf32_biquad_workspace_size(size_t stages);
sigf32_status_t sigf32_biquad_init(sigf32_biquad_t *filter,
                                   const sigf32_biquad_coeffs_t *coeffs,
                                   size_t stages, sigf32_biquad_state_t *state,
                                   size_t state_count);
void sigf32_biquad_reset(sigf32_biquad_t *filter);
float sigf32_biquad_process(sigf32_biquad_t *filter, float input);

typedef struct {
    float mean, rms, minimum, maximum, peak, peak_to_peak, crest_factor;
    uint16_t valid;
} sigf32_stats_result_t;
sigf32_status_t sigf32_stats(const float *input, size_t count, sigf32_stats_result_t *result);

typedef struct {
    float frequency_hz, period_samples;
    uint32_t crossings;
    uint16_t valid;
} sigf32_frequency_result_t;
sigf32_status_t sigf32_frequency_zero_cross(const float *input, size_t count,
                                            float sample_rate_hz, float threshold,
                                            sigf32_frequency_result_t *result);

typedef struct {
    float delay_samples, correlation, amplitude_ratio, phase_pu;
    uint16_t valid;
} sigf32_correlation_result_t;
sigf32_status_t sigf32_cross_correlate(const float *x, const float *y, size_t count,
                                       int max_lag, sigf32_correlation_result_t *result);

typedef enum { SIGF32_WINDOW_RECT = 0, SIGF32_WINDOW_HANN,
               SIGF32_WINDOW_BLACKMAN_HARRIS, SIGF32_WINDOW_FLAT_TOP } sigf32_window_t;
float sigf32_window_value(sigf32_window_t window, size_t index, size_t count);
sigf32_status_t sigf32_window_metrics(sigf32_window_t window, size_t count,
                                     float *coherent_gain, float *enbw_bins);
void sigf32_apply_window(const float *input, float *output, size_t count,
                         sigf32_window_t window);

typedef struct {
    float real, imag, power, amplitude_peak, phase_pu;
    uint16_t valid;
} sigf32_goertzel_result_t;
sigf32_status_t sigf32_goertzel(const float *input, size_t count, float sample_rate_hz,
                                float target_hz, sigf32_goertzel_result_t *result);
sigf32_status_t sigf32_goertzel_multi(const float *input, size_t count,
                                      float sample_rate_hz, const float *targets_hz,
                                      sigf32_goertzel_result_t *results, size_t target_count);

typedef struct {
    float fundamental_hz, fundamental_amplitude, fundamental_phase_pu;
    float thd, thdn, snr_db, sinad_db, sfdr_db, noise_floor_dbfs;
    uint16_t valid;
} sigf32_spectrum_metrics_t;
size_t sigf32_spectrum_workspace_size(size_t fft_size);
sigf32_status_t sigf32_spectrum_analyze(const float *input, size_t count,
                                        float sample_rate_hz, sigf32_window_t window,
                                        unsigned harmonic_count, float *workspace,
                                        size_t workspace_count,
                                        sigf32_spectrum_metrics_t *result);
float sigf32_phase_unwrap(float previous_rad, float current_rad);

typedef struct { float phase_pu, step_pu, sample_rate_hz; sigf32_backend_t backend; } sigf32_nco_t;
sigf32_status_t sigf32_nco_init(sigf32_nco_t *nco, float sample_rate_hz,
                                float frequency_hz, float phase_pu);
sigf32_status_t sigf32_nco_set_frequency(sigf32_nco_t *nco, float frequency_hz);
void sigf32_nco_next(sigf32_nco_t *nco, float *sine, float *cosine);

typedef struct {
    sigf32_nco_t nco;
    float alpha, i_lp, q_lp;
} sigf32_iq_demod_t;
typedef struct { float i, q, amplitude_peak, phase_pu; uint16_t valid; } sigf32_iq_result_t;
sigf32_status_t sigf32_iq_init(sigf32_iq_demod_t *demod, float sample_rate_hz,
                               float carrier_hz, float lowpass_alpha);
sigf32_iq_result_t sigf32_iq_process(sigf32_iq_demod_t *demod, float sample);

typedef struct {
    float sample_rate_hz, phase_pu, nominal_hz, frequency_hz;
    float kp, ki, integrator, min_hz, max_hz, lock_metric;
    float i_lp, q_lp, i_lp2, q_lp2, detector_alpha;
    uint32_t lock_count;
} sigf32_pll_t;
sigf32_status_t sigf32_pll_init(sigf32_pll_t *pll, float sample_rate_hz,
                                float initial_hz, float kp, float ki,
                                float min_hz, float max_hz);
float sigf32_pll_process(sigf32_pll_t *pll, float sample);
float sigf32_pll_frequency_hz(const sigf32_pll_t *pll);
uint16_t sigf32_pll_locked(const sigf32_pll_t *pll);

typedef struct {
    float sample_rate_hz, frequency_hz, smoothing, min_hz, max_hz;
    uint16_t locked;
} sigf32_fll_t;
sigf32_status_t sigf32_fll_init(sigf32_fll_t *fll, float sample_rate_hz,
                                float initial_hz, float smoothing,
                                float min_hz, float max_hz);
sigf32_status_t sigf32_fll_update_block(sigf32_fll_t *fll, const float *input,
                                        size_t count, float threshold);

typedef struct {
    float *weights, *history;
    size_t taps, index;
    float mu, epsilon;
    uint16_t normalized;
} sigf32_lms_t;
size_t sigf32_lms_workspace_size(size_t taps);
sigf32_status_t sigf32_lms_init(sigf32_lms_t *lms, float *weights, float *history,
                                size_t taps, float mu, uint16_t normalized);
void sigf32_lms_reset(sigf32_lms_t *lms);
float sigf32_lms_process(sigf32_lms_t *lms, float reference, float desired,
                         float *error);

typedef struct { float real, imag, magnitude, phase_pu, coherence; uint16_t valid; }
    sigf32_transfer_result_t;
sigf32_status_t sigf32_transfer_point(const float *input, const float *output,
                                      size_t count, float sample_rate_hz,
                                      float frequency_hz, sigf32_transfer_result_t *result);

#ifdef __cplusplus
}
#endif
#endif
