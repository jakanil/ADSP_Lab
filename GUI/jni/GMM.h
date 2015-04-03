#ifndef GMM_H
#define GMM_H

#include <stdlib.h>
#include <math.h>
#include "Constants.h"

typedef struct GMM {
		float*** sigmaInverse;
		float** mu;
		float* prior;
		float* mulConstant;
		float* dm;
		float probability;
} GMM;

GMM* newGMM();
void computeProbabilityGMM(GMM* gmm, float* input);
void destroyGMM(GMM** gmm);

#endif
