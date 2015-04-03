#ifndef FIR_H
#define FIR_H
#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <math.h>

typedef struct FIRFilter {
		int numCoefficients;
		int frameSize;
		float* coefficients;
		float* window;
		float* result;
} FIRFilter;

FIRFilter* newFIR(int frameSize, int numCoefficients, float* coefficients);
void computeFIR(FIRFilter* fir, float* input);
void destroyFIR(FIRFilter** fir);

#endif
