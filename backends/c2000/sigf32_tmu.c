#include "sigf32_tmu.h"
#include "sigf32.h"
#include <math.h>

void sigf32_tmu_sin_cos_pu(float phase_pu, float *sine, float *cosine)
{
    if (!sine || !cosine) return;
    phase_pu = sigf32_phase_wrap_pu(phase_pu);
#if defined(__TMS320C28XX__)
    /* C28x compiler maps these intrinsics to TMU0 when --tmu_support=tmu0. */
    *sine = __sinpuf32(phase_pu);
    *cosine = __cospuf32(phase_pu);
#else
    *sine = sinf(SIGF32_TWO_PI * phase_pu);
    *cosine = cosf(SIGF32_TWO_PI * phase_pu);
#endif
}
