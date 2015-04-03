#ifndef MFCC_H
#define MFCC_H

#include <stdlib.h>
#include <math.h>
#include "Constants.h"

#ifdef LOGCAT_DEBUG
	#include <android/log.h>
#endif

typedef struct MelFCC {
		int NFilters;
		int NFFTFreqs;
		float** MfccFilterWeights;
		float** MfccDCTMatrix;
		float* lifterWeights;
		float* MelFFT;
		float* MelCoeffOut;
} MelFCC;

MelFCC* newMFCC(int frequency, int fftSize);
void computeMFCC(MelFCC* mfcc, float* fftpower);
void destroyMFCC(MelFCC** mfcc);

#endif
