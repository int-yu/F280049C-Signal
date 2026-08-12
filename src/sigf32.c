#include "sigf32.h"

#include <float.h>
#include <math.h>
#include <string.h>

static float sigf32_clampf(float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

float sigf32_phase_wrap_pu(float p) { p -= floorf(p); return p < 0.0f ? p + 1.0f : p; }
float sigf32_phase_pu_to_rad(float p) { return sigf32_phase_wrap_pu(p) * SIGF32_TWO_PI; }
float sigf32_phase_pu_to_deg(float p) { return sigf32_phase_wrap_pu(p) * 360.0f; }
float sigf32_linear_calibrate(float x, float gain, float offset) { return x * gain + offset; }
float sigf32_median3(float a, float b, float c)
{
    if (a > b) { float t = a; a = b; b = t; }
    if (b > c) { float t = b; b = c; c = t; }
    return a > b ? a : b;
}

sigf32_status_t sigf32_dc_blocker_init(sigf32_dc_blocker_t *s, float alpha)
{
    if (!s || alpha < 0.0f || alpha >= 1.0f) return SIGF32_EINVAL;
    s->alpha = alpha; s->x1 = 0.0f; s->y1 = 0.0f; return SIGF32_OK;
}
void sigf32_dc_blocker_reset(sigf32_dc_blocker_t *s) { if (s) { s->x1 = 0; s->y1 = 0; } }
float sigf32_dc_blocker_process(sigf32_dc_blocker_t *s, float x, sigf32_diag_t *d)
{
    if (!s) { if (d) d->invalid_count++; return 0.0f; }
    float y = x - s->x1 + s->alpha * s->y1; s->x1 = x; s->y1 = y;
    if (d) d->valid = isfinite(y) ? 1U : 0U;
    return y;
}

size_t sigf32_moving_average_workspace_size(size_t n) { return n * sizeof(float); }
sigf32_status_t sigf32_moving_average_init(sigf32_moving_average_t *f, float *state,
                                           size_t state_count, size_t n)
{
    if (!f || !state || !n) return SIGF32_EINVAL;
    if (state_count < n) return SIGF32_EWORKSPACE;
    f->state=state; f->length=n; f->index=0; f->sum=0.0f;
    memset(state,0,n*sizeof(*state)); return SIGF32_OK;
}
void sigf32_moving_average_reset(sigf32_moving_average_t *f)
{ if(f){memset(f->state,0,f->length*sizeof(*f->state));f->index=0;f->sum=0.0f;} }
float sigf32_moving_average_process(sigf32_moving_average_t *f,float x)
{ if(!f||!f->state||!f->length)return 0.0f;f->sum+=x-f->state[f->index];f->state[f->index]=x;f->index=(f->index+1U)%f->length;return f->sum/(float)f->length; }

sigf32_status_t sigf32_agc_init(sigf32_agc_t *a,float target,float attack,float release,float min_gain,float max_gain)
{
    if(!a||target<=0.0f||attack<=0.0f||attack>1.0f||release<=0.0f||release>1.0f||min_gain<=0.0f||max_gain<min_gain)return SIGF32_EINVAL;
    a->target_peak=target;a->attack=attack;a->release=release;a->min_gain=min_gain;a->max_gain=max_gain;a->gain=1.0f;a->envelope=0.0f;return SIGF32_OK;
}
void sigf32_agc_reset(sigf32_agc_t *a){if(a){a->gain=1.0f;a->envelope=0.0f;}}
float sigf32_agc_process(sigf32_agc_t *a,float x,sigf32_diag_t *d)
{
    if(!a){if(d)d->invalid_count++;return 0.0f;}float ax=fabsf(x*a->gain);float rate=ax>a->envelope?a->attack:a->release;a->envelope+=rate*(ax-a->envelope);
    if(a->envelope>FLT_EPSILON){float desired=sigf32_clampf(a->gain*a->target_peak/a->envelope,a->min_gain,a->max_gain);a->gain+=rate*(desired-a->gain);}if(d)d->valid=1U;return x*a->gain;
}
size_t sigf32_fir_workspace_size(size_t taps) { return taps * sizeof(float); }
sigf32_status_t sigf32_fir_init(sigf32_fir_t *f, const float *c, size_t n,
                                float *state, size_t state_count)
{
    if (!f || !c || !state || n == 0) return SIGF32_EINVAL;
    if (state_count < n) return SIGF32_EWORKSPACE;
    f->coeffs = c; f->state = state; f->taps = n; f->index = 0;
    memset(state, 0, n * sizeof(*state)); return SIGF32_OK;
}
void sigf32_fir_reset(sigf32_fir_t *f) { if (f) { memset(f->state, 0, f->taps*sizeof(float)); f->index=0; } }
float sigf32_fir_process(sigf32_fir_t *f, float x)
{
    f->state[f->index] = x; float y = 0.0f; size_t p = f->index;
    for (size_t k=0;k<f->taps;k++) { y += f->coeffs[k]*f->state[p]; p = p ? p-1 : f->taps-1; }
    f->index = (f->index + 1U) % f->taps; return y;
}
sigf32_status_t sigf32_fir_process_block(sigf32_fir_t *f,const float *in,float *out,size_t n,sigf32_diag_t *d)
{
    if(!f||!in||!out) return SIGF32_EINVAL;
    for(size_t i=0;i<n;i++) out[i]=sigf32_fir_process(f,in[i]);
    if(d)d->valid=1;
    return SIGF32_OK;
}

sigf32_status_t sigf32_decimator_init(sigf32_decimator_t *d,const float *c,size_t taps,float *state,size_t state_count,size_t factor)
{if(!d||factor<2U)return SIGF32_EINVAL;sigf32_status_t s=sigf32_fir_init(&d->fir,c,taps,state,state_count);if(s!=SIGF32_OK)return s;d->factor=factor;d->phase=0U;return SIGF32_OK;}
void sigf32_decimator_reset(sigf32_decimator_t *d){if(d){sigf32_fir_reset(&d->fir);d->phase=0U;}}
uint16_t sigf32_decimator_process(sigf32_decimator_t *d,float x,float *output)
{if(!d||!output)return 0U;float y=sigf32_fir_process(&d->fir,x);d->phase++;if(d->phase<d->factor)return 0U;d->phase=0U;*output=y;return 1U;}
size_t sigf32_biquad_workspace_size(size_t stages){return stages*sizeof(sigf32_biquad_state_t);}
sigf32_status_t sigf32_biquad_init(sigf32_biquad_t *f,const sigf32_biquad_coeffs_t *c,size_t n,sigf32_biquad_state_t *s,size_t sn)
{if(!f||!c||!s||!n)return SIGF32_EINVAL;if(sn<n)return SIGF32_EWORKSPACE;f->coeffs=c;f->state=s;f->stages=n;memset(s,0,n*sizeof(*s));return SIGF32_OK;}
void sigf32_biquad_reset(sigf32_biquad_t *f){if(f)memset(f->state,0,f->stages*sizeof(*f->state));}
float sigf32_biquad_process(sigf32_biquad_t *f,float x)
{for(size_t i=0;i<f->stages;i++){const sigf32_biquad_coeffs_t*c=&f->coeffs[i];sigf32_biquad_state_t*s=&f->state[i];float y=c->b0*x+s->d1;s->d1=c->b1*x-c->a1*y+s->d2;s->d2=c->b2*x-c->a2*y;x=y;}return x;}

sigf32_status_t sigf32_stats(const float *x,size_t n,sigf32_stats_result_t*r)
{
    if(!x||!r||!n)return SIGF32_EINVAL;
    float sum=0,sq=0;float mn=x[0],mx=x[0],pk=fabsf(x[0]);
    for(size_t i=0;i<n;i++){sum+=x[i];sq+=x[i]*x[i];if(x[i]<mn)mn=x[i];if(x[i]>mx)mx=x[i];if(fabsf(x[i])>pk)pk=fabsf(x[i]);}
    r->mean=(float)(sum/n);r->rms=sqrtf((float)(sq/n));r->minimum=mn;r->maximum=mx;r->peak=pk;r->peak_to_peak=mx-mn;r->crest_factor=r->rms>FLT_MIN?pk/r->rms:0;r->valid=r->rms>FLT_MIN;return r->valid?SIGF32_OK:SIGF32_ENOSIGNAL;
}

sigf32_status_t sigf32_frequency_zero_cross(const float*x,size_t n,float fs,float th,sigf32_frequency_result_t*r)
{
    if(!x||!r||n<3||fs<=0||th<0)return SIGF32_EINVAL;
    float first=0,last=0;uint32_t c=0;
    for(size_t i=1;i<n;i++){if(x[i-1]<=0&&x[i]>0&&(fabsf(x[i]-x[i-1])>=th)){float den=x[i]-x[i-1];float pos=(i-1)+(-x[i-1]/den);if(!c)first=pos;last=pos;c++;}}
    r->crossings=c;r->valid=c>=2;if(!r->valid){r->frequency_hz=0;r->period_samples=0;return SIGF32_ENOSIGNAL;}r->period_samples=(float)((last-first)/(c-1));r->frequency_hz=fs/r->period_samples;return SIGF32_OK;
}

sigf32_status_t sigf32_cross_correlate(const float *x, const float *y, size_t n,
                                       int max_lag, sigf32_correlation_result_t *r)
{
    if (!x || !y || !r || !n || max_lag < 0) return SIGF32_EINVAL;
    float best = -FLT_MAX, best_x_energy = 0.0f, best_y_energy = 0.0f;
    float left = -FLT_MAX, right = -FLT_MAX;
    int best_lag = 0;
    for (int lag = -max_lag; lag <= max_lag; ++lag) {
        float sum = 0.0f, x_energy = 0.0f, y_energy = 0.0f;
        for (size_t i = 0; i < n; ++i) {
            int j = (int)i + lag;
            if (j >= 0 && j < (int)n) {
                sum += x[i] * y[j]; x_energy += x[i] * x[i]; y_energy += y[j] * y[j];
            }
        }
        if (x_energy > 0.0f && y_energy > 0.0f) {
            float correlation = sum / sqrtf(x_energy * y_energy);
            if (correlation > best) {
                best = correlation; best_lag = lag;
                best_x_energy = x_energy; best_y_energy = y_energy;
            }
        }
    }
    if (best_lag > -max_lag && best_lag < max_lag) {
        int neighbor_lags[2] = {best_lag - 1, best_lag + 1};
        float *neighbors[2] = {&left, &right};
        for (unsigned side = 0U; side < 2U; ++side) {
            float sum = 0.0f, xe = 0.0f, ye = 0.0f;
            for (size_t i = 0; i < n; ++i) {
                int j = (int)i + neighbor_lags[side];
                if (j >= 0 && j < (int)n) { sum += x[i]*y[j]; xe += x[i]*x[i]; ye += y[j]*y[j]; }
            }
            if (xe > 0.0f && ye > 0.0f) *neighbors[side] = sum / sqrtf(xe * ye);
        }
    }
    float fraction = 0.0f;
    if (left > -FLT_MAX && right > -FLT_MAX) {
        float denominator = left - 2.0f * best + right;
        if (fabsf(denominator) > FLT_MIN) fraction = 0.5f * (left - right) / denominator;
        fraction = sigf32_clampf(fraction, -0.5f, 0.5f);
    }
    r->delay_samples = (float)best_lag + fraction;
    r->correlation = best;
    r->amplitude_ratio = best_x_energy > 0.0f ? sqrtf(best_y_energy / best_x_energy) : 0.0f;
    r->phase_pu = 0.0f; r->valid = best > -FLT_MAX;
    return r->valid ? SIGF32_OK : SIGF32_ENOSIGNAL;
}
float sigf32_window_value(sigf32_window_t w,size_t i,size_t n)
{
    if(n<=1)return 1;
    float a=SIGF32_TWO_PI*(float)i/(float)(n-1);switch(w){case SIGF32_WINDOW_HANN:return .5f-.5f*cosf(a);case SIGF32_WINDOW_BLACKMAN_HARRIS:return .35875f-.48829f*cosf(a)+.14128f*cosf(2*a)-.01168f*cosf(3*a);case SIGF32_WINDOW_FLAT_TOP:return .21557895f-.41663158f*cosf(a)+.277263158f*cosf(2*a)-.083578947f*cosf(3*a)+.006947368f*cosf(4*a);default:return 1;}}
sigf32_status_t sigf32_window_metrics(sigf32_window_t w,size_t n,float*cg,float*enbw)
{if(!cg||!enbw||!n)return SIGF32_EINVAL;float s=0,s2=0;for(size_t i=0;i<n;i++){float v=sigf32_window_value(w,i,n);s+=v;s2+=v*v;}*cg=(float)(s/n);*enbw=(float)(n*s2/(s*s));return SIGF32_OK;}
void sigf32_apply_window(const float*in,float*out,size_t n,sigf32_window_t w){if(in&&out)for(size_t i=0;i<n;i++)out[i]=in[i]*sigf32_window_value(w,i,n);}

sigf32_status_t sigf32_goertzel(const float*x,size_t n,float fs,float f,sigf32_goertzel_result_t*r)
{
    if(!x||!r||!n||fs<=0||f<0||f>fs*.5f)return SIGF32_EINVAL;
    float om=SIGF32_TWO_PI*f/fs,co=2*cosf(om),s1=0,s2=0;
    for(size_t i=0;i<n;i++){float s=x[i]+co*s1-s2;s2=s1;s1=s;}r->real=s1-s2*cosf(om);r->imag=s2*sinf(om);r->power=r->real*r->real+r->imag*r->imag;r->amplitude_peak=2*sqrtf(r->power)/(float)n;r->phase_pu=sigf32_phase_wrap_pu(atan2f(r->imag,r->real)/SIGF32_TWO_PI);r->valid=r->power>FLT_MIN;return r->valid?SIGF32_OK:SIGF32_ENOSIGNAL;
}
sigf32_status_t sigf32_goertzel_multi(const float*x,size_t n,float fs,const float*f,sigf32_goertzel_result_t*r,size_t m)
{if(!f||!r||!m)return SIGF32_EINVAL;sigf32_status_t st=SIGF32_OK;for(size_t i=0;i<m;i++){sigf32_status_t q=sigf32_goertzel(x,n,fs,f[i],&r[i]);if(q!=SIGF32_OK)st=q;}return st;}

size_t sigf32_spectrum_workspace_size(size_t n){return (n/2+1)*2*sizeof(float);}
sigf32_status_t sigf32_spectrum_analyze(const float *x, size_t n, float fs,
                                        sigf32_window_t w, unsigned harmonic_count,
                                        float *workspace, size_t workspace_count,
                                        sigf32_spectrum_metrics_t *result)
{
    if (!x || !workspace || !result || n < 8U || fs <= 0.0f) return SIGF32_EINVAL;
    size_t bins = n / 2U + 1U;
    if (workspace_count < bins * 2U) return SIGF32_EWORKSPACE;
    float *re = workspace;
    float *im = workspace + bins;
    float coherent_gain, enbw;
    (void)sigf32_window_metrics(w, n, &coherent_gain, &enbw);
    size_t peak = 1U;
    float peak_power = 0.0f;
    for (size_t k = 0; k < bins; ++k) {
        float real = 0.0f, imag = 0.0f;
        for (size_t j = 0; j < n; ++j) {
            float value = x[j] * sigf32_window_value(w, j, n);
            float angle = SIGF32_TWO_PI * (float)k * (float)j / (float)n;
            real += value * cosf(angle);
            imag -= value * sinf(angle);
        }
        re[k] = real; im[k] = imag;
        float power = real * real + imag * imag;
        if (k > 0U && k + 1U < bins && power > peak_power) {
            peak_power = power; peak = k;
        }
    }
    float delta = 0.0f;
    if (peak > 0U && peak + 1U < bins) {
        float left = hypotf(re[peak - 1U], im[peak - 1U]);
        float center = hypotf(re[peak], im[peak]);
        float right = hypotf(re[peak + 1U], im[peak + 1U]);
        float denominator = left - 2.0f * center + right;
        if (fabsf(denominator) > FLT_MIN) delta = 0.5f * (left - right) / denominator;
    }
    size_t radius = w == SIGF32_WINDOW_RECT ? 0U :
                    (w == SIGF32_WINDOW_HANN ? 1U :
                    (w == SIGF32_WINDOW_BLACKMAN_HARRIS ? 3U : 4U));
    float fundamental_power = 0.0f, harmonic_power = 0.0f;
    for (size_t k = peak > radius ? peak - radius : 1U;
         k < bins && k <= peak + radius; ++k)
        fundamental_power += re[k] * re[k] + im[k] * im[k];
    for (unsigned h = 2U; h <= harmonic_count; ++h) {
        long center = lroundf(((float)peak + delta) * (float)h);
        if (center <= 0 || center >= (long)bins) continue;
        long first = center - (long)radius;
        long last = center + (long)radius;
        if (first < 1) first = 1;
        if (last >= (long)bins) last = (long)bins - 1;
        for (long k = first; k <= last; ++k)
            harmonic_power += re[k] * re[k] + im[k] * im[k];
    }
    float noise_power = 0.0f, largest_spur = 0.0f;
    for (size_t k = 1U; k < bins; ++k) {
        long distance = (long)k - (long)peak;
        if (labs(distance) <= (long)radius) continue;
        uint16_t is_harmonic = 0U;
        for (unsigned h = 2U; h <= harmonic_count; ++h) {
            long center = lroundf(((float)peak + delta) * (float)h);
            if (labs((long)k - center) <= (long)radius) { is_harmonic = 1U; break; }
        }
        float power = re[k] * re[k] + im[k] * im[k];
        if (!is_harmonic) noise_power += power;
        if (power > largest_spur) largest_spur = power;
    }
    if (fundamental_power <= FLT_MIN) { result->valid = 0U; return SIGF32_ENOSIGNAL; }
    float safe_noise = fmaxf(noise_power, FLT_MIN);
    float central_amplitude = 2.0f * sqrtf(fundamental_power) /
                              ((float)n * coherent_gain * sqrtf(enbw));
    result->fundamental_hz = ((float)peak + delta) * fs / (float)n;
    result->fundamental_amplitude = central_amplitude;
    result->fundamental_phase_pu = sigf32_phase_wrap_pu(atan2f(im[peak], re[peak]) / SIGF32_TWO_PI);
    result->thd = sqrtf(harmonic_power / fundamental_power);
    result->thdn = sqrtf((harmonic_power + noise_power) / fundamental_power);
    result->snr_db = 10.0f * log10f(fundamental_power / safe_noise);
    result->sinad_db = 10.0f * log10f(fundamental_power /
                                      fmaxf(harmonic_power + noise_power, FLT_MIN));
    result->sfdr_db = 10.0f * log10f(peak_power / fmaxf(largest_spur, FLT_MIN));
    result->noise_floor_dbfs = 10.0f * log10f(safe_noise /
                                             ((float)n * (float)n * coherent_gain * coherent_gain));
    result->valid = 1U;
    return SIGF32_OK;
}
float sigf32_phase_unwrap(float p,float c){while(c-p>SIGF32_PI)c-=SIGF32_TWO_PI;while(c-p<-SIGF32_PI)c+=SIGF32_TWO_PI;return c;}

sigf32_status_t sigf32_nco_init(sigf32_nco_t*n,float fs,float f,float p){if(!n||fs<=0||fabsf(f)>fs*.5f)return SIGF32_EINVAL;n->sample_rate_hz=fs;n->phase_pu=sigf32_phase_wrap_pu(p);n->backend=SIGF32_BACKEND_PORTABLE;return sigf32_nco_set_frequency(n,f);}
sigf32_status_t sigf32_nco_set_frequency(sigf32_nco_t*n,float f){if(!n||n->sample_rate_hz<=0||fabsf(f)>n->sample_rate_hz*.5f)return SIGF32_ERANGE;n->step_pu=f/n->sample_rate_hz;return SIGF32_OK;}
void sigf32_nco_next(sigf32_nco_t*n,float*s,float*c){if(!n||!s||!c)return;float a=SIGF32_TWO_PI*n->phase_pu;*s=sinf(a);*c=cosf(a);n->phase_pu=sigf32_phase_wrap_pu(n->phase_pu+n->step_pu);}

sigf32_status_t sigf32_iq_init(sigf32_iq_demod_t*d,float fs,float f,float a){if(!d||a<=0||a>1)return SIGF32_EINVAL;sigf32_status_t s=sigf32_nco_init(&d->nco,fs,f,0);d->alpha=a;d->i_lp=d->q_lp=0;return s;}
sigf32_iq_result_t sigf32_iq_process(sigf32_iq_demod_t*d,float x){sigf32_iq_result_t r={0};if(!d)return r;float s,c;sigf32_nco_next(&d->nco,&s,&c);d->i_lp+=d->alpha*(x*c-d->i_lp);d->q_lp+=d->alpha*(x*(-s)-d->q_lp);r.i=d->i_lp;r.q=d->q_lp;r.amplitude_peak=2*hypotf(r.i,r.q);r.phase_pu=sigf32_phase_wrap_pu(atan2f(r.q,r.i)/SIGF32_TWO_PI);r.valid=r.amplitude_peak>FLT_MIN;return r;}

sigf32_status_t sigf32_pll_init(sigf32_pll_t*p,float fs,float f,float kp,float ki,float lo,float hi){if(!p||fs<=0||lo<0||hi<=lo||f<lo||f>hi||kp<0||ki<0)return SIGF32_EINVAL;memset(p,0,sizeof(*p));p->sample_rate_hz=fs;p->nominal_hz=f;p->frequency_hz=f;p->kp=kp;p->ki=ki;p->min_hz=lo;p->max_hz=hi;p->detector_alpha=0.06f;return SIGF32_OK;}
float sigf32_pll_process(sigf32_pll_t *p, float x)
{
    float angle = SIGF32_TWO_PI * p->phase_pu;
    float mixed_i = 2.0f * x * sinf(angle);
    float mixed_q = 2.0f * x * cosf(angle);
    p->i_lp += p->detector_alpha * (mixed_i - p->i_lp);
    p->q_lp += p->detector_alpha * (mixed_q - p->q_lp);
    p->i_lp2 += p->detector_alpha * (p->i_lp - p->i_lp2);
    p->q_lp2 += p->detector_alpha * (p->q_lp - p->q_lp2);
    float err_pu = atan2f(p->q_lp2, p->i_lp2) / SIGF32_TWO_PI;
    float min_step = (p->min_hz - p->nominal_hz) / p->sample_rate_hz;
    float max_step = (p->max_hz - p->nominal_hz) / p->sample_rate_hz;
    p->integrator = sigf32_clampf(p->integrator + p->ki * err_pu,
                                  min_step, max_step);
    float step = p->nominal_hz / p->sample_rate_hz + p->integrator +
                 p->kp * err_pu;
    p->phase_pu = sigf32_phase_wrap_pu(p->phase_pu + step);
    p->frequency_hz = sigf32_clampf(p->nominal_hz +
                                    (p->integrator + p->kp * err_pu) *
                                    p->sample_rate_hz,
                                    p->min_hz, p->max_hz);
    p->lock_metric = 0.995f * p->lock_metric + 0.005f * fabsf(err_pu);
    if (p->lock_metric < 0.01f) p->lock_count++;
    else p->lock_count = 0;
    return err_pu;
}float sigf32_pll_frequency_hz(const sigf32_pll_t*p){return p?p->frequency_hz:0;}
uint16_t sigf32_pll_locked(const sigf32_pll_t*p){return p&&p->lock_count>100;}

sigf32_status_t sigf32_fll_init(sigf32_fll_t *f,float fs,float initial,float smoothing,float lo,float hi)
{if(!f||fs<=0.0f||smoothing<=0.0f||smoothing>1.0f||lo<=0.0f||hi<=lo||initial<lo||initial>hi)return SIGF32_EINVAL;f->sample_rate_hz=fs;f->frequency_hz=initial;f->smoothing=smoothing;f->min_hz=lo;f->max_hz=hi;f->locked=0U;return SIGF32_OK;}
sigf32_status_t sigf32_fll_update_block(sigf32_fll_t *f,const float *x,size_t n,float threshold)
{if(!f||!x||n<3U||threshold<0.0f)return SIGF32_EINVAL;sigf32_frequency_result_t r;sigf32_status_t s=sigf32_frequency_zero_cross(x,n,f->sample_rate_hz,threshold,&r);if(s!=SIGF32_OK){f->locked=0U;return s;}if(r.frequency_hz<f->min_hz||r.frequency_hz>f->max_hz){f->locked=0U;return SIGF32_ERANGE;}f->frequency_hz+=f->smoothing*(r.frequency_hz-f->frequency_hz);f->locked=(uint16_t)(fabsf(r.frequency_hz-f->frequency_hz)<=0.005f*fmaxf(r.frequency_hz,1.0f));return SIGF32_OK;}
size_t sigf32_lms_workspace_size(size_t n){return 2*n*sizeof(float);}
sigf32_status_t sigf32_lms_init(sigf32_lms_t*l,float*w,float*h,size_t n,float mu,uint16_t norm){if(!l||!w||!h||!n||mu<=0||mu>=2)return SIGF32_EINVAL;l->weights=w;l->history=h;l->taps=n;l->index=0;l->mu=mu;l->epsilon=1e-9f;l->normalized=norm;memset(w,0,n*sizeof(*w));memset(h,0,n*sizeof(*h));return SIGF32_OK;}
void sigf32_lms_reset(sigf32_lms_t*l){if(l){memset(l->weights,0,l->taps*sizeof(float));memset(l->history,0,l->taps*sizeof(float));l->index=0;}}
float sigf32_lms_process(sigf32_lms_t*l,float x,float d,float*ep){l->history[l->index]=x;float y=0,e2=0;size_t p=l->index;for(size_t k=0;k<l->taps;k++){float v=l->history[p];y+=l->weights[k]*v;e2+=v*v;p=p?p-1:l->taps-1;}float e=d-y;float mu=l->normalized?l->mu/(l->epsilon+e2):l->mu;p=l->index;for(size_t k=0;k<l->taps;k++){l->weights[k]+=mu*e*l->history[p];p=p?p-1:l->taps-1;}l->index=(l->index+1)%l->taps;if(ep)*ep=e;return y;}

sigf32_status_t sigf32_transfer_point(const float*x,const float*y,size_t n,float fs,float f,sigf32_transfer_result_t*r){if(!r)return SIGF32_EINVAL;sigf32_goertzel_result_t a,b;sigf32_status_t s=sigf32_goertzel(x,n,fs,f,&a);if(s!=SIGF32_OK)return s;s=sigf32_goertzel(y,n,fs,f,&b);if(s!=SIGF32_OK)return s;float den=a.real*a.real+a.imag*a.imag;if(den<=FLT_MIN)return SIGF32_ENOSIGNAL;r->real=(b.real*a.real+b.imag*a.imag)/den;r->imag=(b.imag*a.real-b.real*a.imag)/den;r->magnitude=hypotf(r->real,r->imag);r->phase_pu=sigf32_phase_wrap_pu(atan2f(r->imag,r->real)/SIGF32_TWO_PI);r->coherence=1;r->valid=1;return SIGF32_OK;}
