#include "MFCC.h"

float hertzToMel(float hertz);
float melToHertz(float mel);

MelFCC*
newMFCC(int frequency, int fftSize)
{
	MelFCC* newMelFCC = (MelFCC*)malloc(sizeof(MelFCC));

	float MLow = hertzToMel(LOWEST_FREQUENCY);
	float MHigh = hertzToMel(HIGHEST_FREQUENCY == 0 ? frequency/2 : HIGHEST_FREQUENCY);
	int NFilters = TOTAL_FILTERS == 0 ? (int)ceil(hertzToMel(MHigh)/2) : TOTAL_FILTERS;
	newMelFCC->NFilters = NFilters;
	int NFFTFreqs = fftSize/2 + 1;
	newMelFCC->NFFTFreqs = NFFTFreqs;

	#ifdef LOGCAT_DEBUG
		__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "MFCC Initialization: NFilters %d, NFFTFreqs %d, MLow %.8f, MHigh %.8f", NFilters, NFFTFreqs, MLow, MHigh);
	#endif

	int i, j;
	float FFTFreq [NFFTFreqs];
	for(i=0;i<NFFTFreqs;i++)
	{
		FFTFreq[i] = i*((float)frequency/fftSize);
		#ifdef LOGCAT_DEBUG
			__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "MFCC Initialization: FFTFreq[%d] - %.8f", i, FFTFreq[i]);
		#endif
	}

	float Freq [NFilters+2];
	float MFilterStep = (MHigh-MLow)/(NFilters+1);
	for(i=0;i<NFilters+2;i++)
	{
		Freq[i] = melToHertz(MLow + i*MFilterStep);
		#ifdef LOGCAT_DEBUG
			__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "MFCC Initialization: Freq[%d] - %.8f", i, Freq[i]);
		#endif
	}

	newMelFCC->MfccFilterWeights = (float**)malloc(NFilters*sizeof(float*));
	for(i=0;i<NFilters;i++)
	{
		newMelFCC->MfccFilterWeights[i] = (float*)malloc(NFFTFreqs*sizeof(float));
	}
	float low = 0;
	float high = 0;
	for(i=0;i<NFilters;i++)
	{
		float constamp = 2.0/(Freq[i+2]-Freq[i]);
		float fs_0 = Freq[i+1]+BWIDTH*(Freq[i]-Freq[i+1]);
		float fs_2 = Freq[i+1]+BWIDTH*(Freq[i+2]-Freq[i+1]);
		for(j=0;j<NFFTFreqs;j++)
		{
			low = ((FFTFreq[j]-fs_0)/(Freq[i+1]-fs_0));
			high = ((fs_2-FFTFreq[j])/(fs_2-Freq[i+1]));
			newMelFCC->MfccFilterWeights[i][j] = (float)(constamp*fmax(0, fmin(low, high)));
			#ifdef LOGCAT_DEBUG
				__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "MFCC Initialization: MFCCFilterWeights[%d,%d] - %.8f", i, j, newMelFCC->MfccFilterWeights[i][j]);
			#endif
		}
	}

	newMelFCC->MfccDCTMatrix = (float**)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float*));
	for(i=0;i<CEPSTRAL_COEFFICIENTS;i++)
	{
		newMelFCC->MfccDCTMatrix[i] = (float*)malloc(NFilters*sizeof(float));
	}
	float temp = sqrt(2.0/NFilters);
	for(i=1;i<CEPSTRAL_COEFFICIENTS;i++)
	{
		for(j=0;j<NFilters;j++) {
			newMelFCC->MfccDCTMatrix[i][j]  = (float)(cos(i*(2*j+1)*M_PI/(2*NFilters))*temp);
			#ifdef LOGCAT_DEBUG
				__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "MFCC Initialization: MFCCDCTMatrix[%d,%d] - %.8f", i, j, newMelFCC->MfccDCTMatrix[i][j]);
			#endif
		}
	}
	temp /= sqrt(2.0);
	for(j=0;j<NFilters;j++)
	{
		newMelFCC->MfccDCTMatrix[0][j] = (float)temp;
	}

	newMelFCC->lifterWeights = (float*)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float));
	newMelFCC->lifterWeights[0] = 1.0f;
	#ifdef LOGCAT_DEBUG
		__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "MFCC Initialization: MFCCLifterWeights[0] - %.8f", newMelFCC->lifterWeights[0]);
	#endif
	for(i=1;i<CEPSTRAL_COEFFICIENTS;i++)
	{
		newMelFCC->lifterWeights[i] = (float)pow(i, LIFT);
		#ifdef LOGCAT_DEBUG
			__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "MFCC Initialization: MFCCLifterWeights[%d] - %.8f", i, newMelFCC->lifterWeights[i]);
		#endif
	}

	newMelFCC->MelFFT = (float*)malloc(NFilters*sizeof(float));
	newMelFCC->MelCoeffOut = (float*)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float));

	return newMelFCC;
}

void
computeMFCC(MelFCC* mfcc, float* fftpower)
{
	int i, j;
	for(i=0;i<(mfcc->NFilters);i++)
	{
		mfcc->MelFFT[i] = 0;
		for(j=0;j<(mfcc->NFFTFreqs);j++)
		{
			mfcc->MelFFT[i] += fftpower[j]*mfcc->MfccFilterWeights[i][j];
		}
	}

	for(i=0;i<(mfcc->NFilters);i++)
	{
		mfcc->MelFFT[i] = (float)log(mfcc->MelFFT[i]);
	}

	for(i=0;i<CEPSTRAL_COEFFICIENTS;i++)
	{
		mfcc->MelCoeffOut[i] = 0;
		for(j=0;j<(mfcc->NFilters);j++)
		{
			mfcc->MelCoeffOut[i] += mfcc->MelFFT[j]*mfcc->MfccDCTMatrix[i][j];
		}
	}

	for(i=0;i<CEPSTRAL_COEFFICIENTS;i++)
	{
		mfcc->MelCoeffOut[i] *=  mfcc->lifterWeights[i];
	}
}

void
destroyMFCC(MelFCC** mfcc)
{
	if(*mfcc != NULL){
		int i;
		if((*mfcc)->MfccFilterWeights != NULL){
			for(i=0;i<(*mfcc)->NFilters;i++){
				free((*mfcc)->MfccFilterWeights[i]);
			}
			free((*mfcc)->MfccFilterWeights);
			(*mfcc)->MfccFilterWeights = NULL;
		}
		if((*mfcc)->MfccDCTMatrix != NULL){
			for(i=0;i<CEPSTRAL_COEFFICIENTS;i++){
				free((*mfcc)->MfccDCTMatrix[i]);
			}
			free((*mfcc)->MfccDCTMatrix);
			(*mfcc)->MfccDCTMatrix = NULL;
		}
		if((*mfcc)->lifterWeights != NULL){
			free((*mfcc)->lifterWeights);
			(*mfcc)->lifterWeights = NULL;
		}
		if((*mfcc)->MelFFT != NULL){
			free((*mfcc)->MelFFT);
			(*mfcc)->MelFFT = NULL;
		}
		if((*mfcc)->MelCoeffOut != NULL){
			free((*mfcc)->MelCoeffOut);
			(*mfcc)->MelCoeffOut = NULL;
		}
		free(*mfcc);
		*mfcc = NULL;
	}
}

float
hertzToMel(float hertz){
	//O'Shaughnessy formula using log base e
	//return 1127*(float)(Math.log(((double)hertz/700)+1));

	if(hertz < BRKFRQ){
		return (hertz - F_0)/(200.0f/3.0f);
	} else {
		return ((BRKFRQ-F_0)/(200.0f/3.0f))+(log(hertz/BRKFRQ))/(log(6.4f)/27.0f);
	}
}

float
melToHertz(float mel){
	//return (((float)(Math.exp((double)mel/1127))-1)*700);

	if(mel < ((BRKFRQ-F_0)/(200.0f/3.0f))){
		return F_0 + (200.0f/3.0f)*mel;
	} else {
		return BRKFRQ*exp((log(6.4f)/27.0f)*(mel-(BRKFRQ-F_0)/(200.0f/3.0f)));
	}
}
