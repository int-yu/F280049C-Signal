#include "sigf32_rfft_adapter.h"
#include <math.h>
sigf32_status_t sigf32_rfft_reference(const float *input,size_t n,float *real,float *imag)
{if(!input||!real||!imag||n<2U)return SIGF32_EINVAL;for(size_t k=0;k<=n/2U;++k){float rr=0.0f,ii=0.0f;for(size_t i=0;i<n;++i){float a=SIGF32_TWO_PI*(float)(i*k)/(float)n;rr+=input[i]*cosf(a);ii-=input[i]*sinf(a);}real[k]=rr;imag[k]=ii;}return SIGF32_OK;}
#if defined(SIGF32_ENABLE_C2000WARE_RFFT)
sigf32_status_t sigf32_rfft_c2000ware(RFFT_F32_STRUCT *object,float *input,float *output,float *twiddles,size_t n,float *real,float *imag)
{if(!object||!input||!output||!twiddles||!real||!imag||n<32U||n>2048U||(n&(n-1U))!=0U)return SIGF32_EINVAL;uint16_t stages=0U;for(size_t v=n;v>1U;v>>=1U)++stages;RFFT_f32_setInputPtr(object,input);RFFT_f32_setOutputPtr(object,output);RFFT_f32_setTwiddlesPtr(object,twiddles);RFFT_f32_setStages(object,stages);RFFT_f32_setFFTSize(object,(uint16_t)n);RFFT_f32_sincostable(object);RFFT_f32(object);real[0]=output[0];imag[0]=0.0f;for(size_t k=1U;k<n/2U;++k){real[k]=output[k];imag[k]=output[n-k];}real[n/2U]=output[n/2U];imag[n/2U]=0.0f;return SIGF32_OK;}
#endif
