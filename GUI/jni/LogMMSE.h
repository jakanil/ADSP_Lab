#ifndef LOGMMSE_h
#define LOGMMSE_h

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Constants.h"

typedef struct LogMMSE {
	int classes;
	int nFFT;
	int frequency;
	int NumWinPmin;
	int runCount;
	int safetyNetIndex;
	float*** gainTable;
	float* gammak;
	float* noiseMu2;
	float* previousXk;
	float* previousRk;
	float* smoothPost;
	float* hw;
	float** safetyNetP;
	float* safetyNetPMinimum;
	float* speechPresenceProbability;
	void (*suppress)(struct LogMMSE* logMMSE, int noiseClass, float* input);
} LogMMSE;

LogMMSE* newLogMMSE(int classes, int nFFT, int frequency, int overlap);
void destroyLogMMSE(LogMMSE** logMMSE);

#endif
