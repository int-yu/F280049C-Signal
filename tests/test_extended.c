#include <math.h>
#include <stdio.h>
#include "sigf32.h"

#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); return 1; } } while (0)

int main(void)
{
    float cg=0,enbw=0;
    CHECK(sigf32_window_metrics(SIGF32_WINDOW_HANN, 1024, &cg, &enbw)==SIGF32_OK);
    CHECK(fabsf(cg-0.5f)<0.002f && fabsf(enbw-1.5f)<0.01f);

    float x[256], y[256], ws[258];
    for(int i=0;i<256;i++){
        float a=SIGF32_TWO_PI*20.0f*i/256.0f;
        x[i]=0.7f*sinf(a)+0.07f*sinf(2*a);
        y[i]=0.35f*sinf(a+SIGF32_PI/4.0f);
    }
    sigf32_spectrum_metrics_t m;
    CHECK(sigf32_spectrum_analyze(x,256,256.0f,SIGF32_WINDOW_HANN,5,ws,258,&m)==SIGF32_OK);
    CHECK(fabsf(m.fundamental_hz-20.0f)<0.1f);
    CHECK(fabsf(m.fundamental_amplitude-0.7f)<0.02f);
    CHECK(fabsf(m.thd-0.1f)<0.03f);
    CHECK(fabsf(m.thdn-0.1f)<0.03f);
    CHECK(fabsf(m.sfdr_db-20.0f)<2.0f);
    for(int i=0;i<256;i++) x[i]=0.65f*sinf(SIGF32_TWO_PI*20.3f*i/256.0f);
    CHECK(sigf32_spectrum_analyze(x,256,256.0f,SIGF32_WINDOW_HANN,5,ws,258,&m)==SIGF32_OK);
    CHECK(fabsf(m.fundamental_hz-20.3f)<0.08f);
    CHECK(fabsf(m.fundamental_amplitude-0.65f)<0.01f);

    for(int i=0;i<256;i++){
        float a=SIGF32_TWO_PI*20.0f*i/256.0f;
        x[i]=0.7f*sinf(a)+0.07f*sinf(2*a);
    }
    sigf32_transfer_result_t tr;
    CHECK(sigf32_transfer_point(x,y,256,256.0f,20.0f,&tr)==SIGF32_OK);
    CHECK(fabsf(tr.magnitude-0.5f)<0.01f);
    CHECK(fabsf(sigf32_phase_pu_to_deg(tr.phase_pu)-45.0f)<0.5f);

    sigf32_iq_demod_t iq;
    CHECK(sigf32_iq_init(&iq,1000.0f,50.0f,0.02f)==SIGF32_OK);
    sigf32_iq_result_t ir={0};
    for(int i=0;i<3000;i++) ir=sigf32_iq_process(&iq,0.8f*sinf(SIGF32_TWO_PI*50*i/1000.0f));
    CHECK(ir.valid && fabsf(ir.amplitude_peak-0.8f)<0.01f);

    float delayed[256]={0};
    for(int i=4;i<256;i++) delayed[i]=x[i-4];
    sigf32_correlation_result_t cr;
    CHECK(sigf32_cross_correlate(x,delayed,256,12,&cr)==SIGF32_OK);
    CHECK(fabsf(cr.delay_samples-4.0f)<0.1f);
    float qa[32]={0}, qb[32]={0};
    qa[10]=1.0f; qb[12]=0.5f; qb[13]=1.0f;
    CHECK(sigf32_cross_correlate(qa,qb,32,6,&cr)==SIGF32_OK);
    CHECK(cr.delay_samples>2.0f && cr.delay_samples<3.0f);
    puts("F280049C extended tests: PASS");
    return 0;
}
