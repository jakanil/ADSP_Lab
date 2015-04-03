#include "FIR.h"

FIRFilter*
newFIR(int frameSize, int numCoefficients, float* coefficients)
{
	FIRFilter* newFIR = (FIRFilter*)malloc(sizeof(FIRFilter));

	newFIR->numCoefficients = numCoefficients;
	newFIR->frameSize = frameSize;
	newFIR->coefficients = (float*)malloc(numCoefficients*sizeof(float));
	newFIR->window = (float*)calloc(numCoefficients+frameSize,sizeof(float));
	newFIR->result = (float*)malloc(frameSize*sizeof(float));

	int i;
	for(i=0;i<numCoefficients;i++)
	{
		newFIR->coefficients[i] = coefficients[i];
	}

	return newFIR;
}

void
computeFIR(FIRFilter* fir, float* input)
{
	int i, j;
	float temp;
	float* BuffPnt = fir->window;
	BuffPnt++;

	for(i=(fir->numCoefficients)-2; i>=0; i--) {
		*BuffPnt=BuffPnt[fir->frameSize];
		BuffPnt++;
	}

	for(i=0;i<fir->frameSize;i++)
	{
		temp = 0;
		*(BuffPnt) = input[i];
		for(j=0;j<(fir->numCoefficients);j++)
		{
			temp += BuffPnt[-j]*fir->coefficients[j];
		}
		BuffPnt++;
		fir->result[i] = temp;
	}
}

void
destroyFIR(FIRFilter** fir)
{
	if(*fir != NULL){
		if((*fir)->coefficients != NULL){
			free((*fir)->coefficients);
			(*fir)->coefficients = NULL;
		}
		if((*fir)->window != NULL){
			free((*fir)->window);
			(*fir)->window = NULL;
		}
		if((*fir)->result != NULL){
			free((*fir)->result);
			(*fir)->result = NULL;
		}
		free(*fir);
		*fir = NULL;
	}
}
