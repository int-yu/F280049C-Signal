#ifndef SIGF32_TMU_H
#define SIGF32_TMU_H

/* C28x TMU0 helper. phase_pu is one cycle in [0, 1), not radians. */
void sigf32_tmu_sin_cos_pu(float phase_pu, float *sine, float *cosine);

#endif
