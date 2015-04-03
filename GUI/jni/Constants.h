#ifndef CONSTANTS_H
#define CONSTANTS_H

	//MelCoeff parameters
	#define LIFT 0.6f
	#define PREEMPH 0.97f
	#define LOWEST_FREQUENCY 0
	#define HIGHEST_FREQUENCY 0 //nyquist
	#define TOTAL_FILTERS 40
	#define CEPSTRAL_COEFFICIENTS 13
	#define F_0 0.0f
	#define BRKFRQ 1000.0f
	#define BWIDTH 1.0f

	//VAD Parameters in VAD.h

	//GMM Parameters
	#define NSTATES 2

	//LogMMSE Parameters
	#define INITIALIZATION_WINDOWS 6
	#define AA 0.98
	#define KSI_MIN 0.00316227766016838
	#define RANGEPRIORMAX 40
	#define RANGEPRIORMIN -19
	#define RANGEPOSTMAX 40
	#define RANGEPOSTMIN -30
	#define TKM 4
	#define ALPHAP 0.1
	#define ALPHAD 0.85
	#define SAFETYNETB 1.5
	#define SAFETYNETETA 0.1

	//Exponential Integral Parameters
	#define MAXIT 100
	#define EULER 0.57721566490153286061
	#define FPMIN 1.0e-30
	#define FPMAX 3.4e+38
	#define EPS 1.0e-7

#endif
