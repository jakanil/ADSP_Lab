#include "LogMMSE.h"
#include "LogMMSEParam.h"

void initializeNoiseMu(LogMMSE* logMMSE, int decisionClass, float* input);
void suppressFirstWindow(LogMMSE* logMMSE, int decisionClass, float* input);
void suppress(LogMMSE* logMMSE, int decisionClass, float* input);
float exponentialIntegral(int n, float x);

LogMMSE*
newLogMMSE(int classes, int nFFT, int frequency, int overlap)
{
	LogMMSE* newLogMMSE = (LogMMSE*)malloc(sizeof(LogMMSE));

	newLogMMSE->classes = classes;
	newLogMMSE->nFFT = nFFT;
	newLogMMSE->frequency = frequency;
	newLogMMSE->NumWinPmin = (int)floor((0.8f*frequency)/overlap);
	newLogMMSE->runCount = 0;
	newLogMMSE->safetyNetIndex = 0;

	newLogMMSE->gammak = (float*)calloc(nFFT,sizeof(float));
	newLogMMSE->noiseMu2 = (float*)calloc(nFFT,sizeof(float));
	newLogMMSE->previousXk = (float*)calloc(nFFT,sizeof(float));
	newLogMMSE->previousRk = (float*)calloc(nFFT,sizeof(float));
	newLogMMSE->smoothPost = (float*)calloc(nFFT,sizeof(float));
	newLogMMSE->hw = (float*)calloc(nFFT,sizeof(float));

	int i, j, k;

	newLogMMSE->safetyNetP = (float**)malloc(nFFT*sizeof(float*));
	for (i=0; i<nFFT; i++){
		newLogMMSE->safetyNetP[i] = (float*)malloc(newLogMMSE->NumWinPmin*sizeof(float));
		for(j=0;j<newLogMMSE->NumWinPmin;j++)
		{
			newLogMMSE->safetyNetP[i][j] = FPMAX;
		}
	}

	newLogMMSE->safetyNetPMinimum = (float*)malloc(nFFT*sizeof(float));

	for(i=0;i<nFFT;i++)
	{
		newLogMMSE->safetyNetPMinimum[i] = FPMAX;
	}

	newLogMMSE->speechPresenceProbability = (float*)malloc(nFFT*sizeof(float));
	newLogMMSE->gainTable = (float***)malloc(classes*sizeof(float**));

	for(i=0;i<classes;i++)
	{
		newLogMMSE->gainTable[i] = (float**)malloc(60*sizeof(float*));
		for(j=0;j<60;j++)
		{
			newLogMMSE->gainTable[i][j] = (float*)malloc(71*sizeof(float));
			for(k=0;k<71;k++)
			{
				newLogMMSE->gainTable[i][j][k] = mmseparam[i][71*j+k];
			}
		}
	}

	newLogMMSE->suppress = initializeNoiseMu;
	return newLogMMSE;
}

void
initializeNoiseMu(LogMMSE* logMMSE, int decisionClass, float* input)
{
	int i;
	for(i=0;i<logMMSE->nFFT;i++)
	{
		logMMSE->noiseMu2[i] += input[i];
	}

	if(++logMMSE->runCount >= INITIALIZATION_WINDOWS){

		for(i=0;i<logMMSE->nFFT;i++)
		{
			logMMSE->noiseMu2[i] = logMMSE->noiseMu2[i]/(INITIALIZATION_WINDOWS*INITIALIZATION_WINDOWS);
		}

		logMMSE->suppress = suppressFirstWindow;
	}
}

void
suppressFirstWindow(LogMMSE* logMMSE, int decisionClass, float* input)
{
	int i, j, ksiNTdB, gammakdB;
	float tmp, ksi, ksiNT, D2est, alphas, A, vk, ei_vk;

	for (i=0; i<logMMSE->nFFT; i++)
	{
		logMMSE->gammak[i] = fmin(input[i] / logMMSE->noiseMu2[i], 40);
	}

	for (i=0; i<logMMSE->nFFT; i++)
	{
		ksi = AA + (1-AA)*fmax(logMMSE->gammak[i]-1, 0);
		ksiNT = fmax((AA + (1-AA)*logMMSE->gammak[i]),KSI_MIN);
		logMMSE->safetyNetP[i][0] = (1-SAFETYNETETA)*input[i];
		logMMSE->safetyNetPMinimum[i] = logMMSE->safetyNetP[i][0];


		ksiNTdB = (int)round(fmax(fmin(10*log10(ksiNT),RANGEPRIORMAX),RANGEPRIORMIN)) + 20;
		gammakdB = (int)round(fmax(fmin(10*log10(logMMSE->gammak[i]),RANGEPOSTMAX),RANGEPOSTMIN)) + 31;

		D2est = logMMSE->gainTable[decisionClass][ksiNTdB-1][gammakdB-1]*input[i];

		logMMSE->speechPresenceProbability[i] = 0.9;

		alphas = ALPHAD + (1-ALPHAD)*logMMSE->speechPresenceProbability[i];
		logMMSE->noiseMu2[i] = alphas*logMMSE->noiseMu2[i] + (1-alphas)*D2est;

		if (SAFETYNETB*logMMSE->safetyNetPMinimum[i] > logMMSE->noiseMu2[i]) {
			logMMSE->speechPresenceProbability[i] = 0;
			logMMSE->noiseMu2[i] = fmax(SAFETYNETB*logMMSE->safetyNetPMinimum[i], logMMSE->noiseMu2[i]);
		}

		A = ksi/(1+ksi);
		vk = A*logMMSE->gammak[i];
		ei_vk = 0.5f*exponentialIntegral(1, vk);
		logMMSE->hw[i] = A*exp(ei_vk);
		logMMSE->previousRk[i] = input[i];
		logMMSE->previousXk[i] = input[i]*logMMSE->hw[i]*logMMSE->hw[i];
	}

	logMMSE->safetyNetIndex++;
	logMMSE->suppress = suppress;
}

void
suppress(LogMMSE* logMMSE, int decisionClass, float* input)
{
	int i, j, ksiNTdB, gammakdB;
	int nFFT = logMMSE->nFFT;

	float tmp, ksi, ksiNT, D2est, alphas, A, vk, ei_vk;

	float* gammak = logMMSE->gammak;

	for (i=0; i<nFFT; i++)
	{
		gammak[i] = fmin(input[i] / logMMSE->noiseMu2[i], 40);
	}

	logMMSE->smoothPost[0] = (gammak[0] + gammak[1])/2.0;
	for(i=1;i<nFFT-1;i++)
	{
		logMMSE->smoothPost[i] = (gammak[i-1] + gammak[i] + gammak[i+1])/3.0;
	}
	logMMSE->smoothPost[nFFT-1] = (gammak[nFFT-1] +gammak[nFFT-2])/2.0;

	for (i=0; i<nFFT; i++)
	{
		ksi = AA*logMMSE->previousXk[i]/logMMSE->noiseMu2[i] + (1-AA)*fmax(logMMSE->gammak[i]-1.0, 0.0);
		ksi = fmax(KSI_MIN,ksi);
		ksiNT = fmax(((AA*logMMSE->previousRk[i]/logMMSE->noiseMu2[i]) + (1.0-AA)*logMMSE->gammak[i]), KSI_MIN);
		logMMSE->previousRk[i] = input[i];

		if (logMMSE->safetyNetIndex > 0) {
			logMMSE->safetyNetP[i][logMMSE->safetyNetIndex] = SAFETYNETETA*logMMSE->safetyNetP[i][logMMSE->safetyNetIndex-1] + (1-SAFETYNETETA)*logMMSE->previousRk[i];
		} else {
			logMMSE->safetyNetP[i][logMMSE->safetyNetIndex] = SAFETYNETETA*logMMSE->safetyNetP[i][logMMSE->NumWinPmin-1] + (1-SAFETYNETETA)*logMMSE->previousRk[i];
		}

		ksiNTdB = (int)round(fmax(fmin(10*log10(ksiNT),RANGEPRIORMAX),RANGEPRIORMIN)) + 20;
		gammakdB = (int)round(fmax(fmin(10*log10(logMMSE->gammak[i]),RANGEPOSTMAX),RANGEPOSTMIN)) + 31;

		D2est = logMMSE->gainTable[decisionClass][ksiNTdB-1][gammakdB-1]*logMMSE->previousRk[i];

		if (logMMSE->smoothPost[i] > TKM) {
			logMMSE->speechPresenceProbability[i] = ALPHAP*logMMSE->speechPresenceProbability[i] + (1-ALPHAP);
		} else {
			logMMSE->speechPresenceProbability[i] = ALPHAP*logMMSE->speechPresenceProbability[i];
		}

		alphas = ALPHAD + (1-ALPHAD)*logMMSE->speechPresenceProbability[i];
		logMMSE->noiseMu2[i] = alphas*logMMSE->noiseMu2[i] + (1-alphas)*D2est;

		tmp = logMMSE->safetyNetP[i][0];
		for (j=1; j<logMMSE->NumWinPmin; j++)
		{
			if (logMMSE->safetyNetP[i][j] < tmp) {
				tmp = logMMSE->safetyNetP[i][j];
			}
		}
		logMMSE->safetyNetPMinimum[i] = tmp;

		if (SAFETYNETB*logMMSE->safetyNetPMinimum[i] > logMMSE->noiseMu2[i]) {
			logMMSE->speechPresenceProbability[i] = 0;
			logMMSE->noiseMu2[i] = fmax(SAFETYNETB*logMMSE->safetyNetPMinimum[i], logMMSE->noiseMu2[i]);
		}

		A = ksi/(1+ksi);
		vk = A*logMMSE->gammak[i];
		ei_vk = exponentialIntegral(1, vk)/2.0;
		logMMSE->hw[i] = A*exp(ei_vk);
		logMMSE->previousXk[i] = logMMSE->previousRk[i]*logMMSE->hw[i]*logMMSE->hw[i];
	}

	logMMSE->safetyNetIndex++;
	if (logMMSE->safetyNetIndex >= logMMSE->NumWinPmin) {
		logMMSE->safetyNetIndex = 0;
	}
}

float
exponentialIntegral(int n, float x)
{
	//from Numerical Recipes
	int i, ii, nm1;
	float a, b, c, d, del, fact, h, psi, ans;

	nm1 = n-1;

	if (n < 0 || x < 0.0f || (x == 0.0f && (n == 0 || n == 1))) {

		return NAN;

	} else if (n == 0) {

		ans = exp(-x)/x;

	} else if (x == 0.0) {

		ans = 1.0f/nm1;

	} else if (x > 1.0f) {

		b = x+n;
		c = 1.0f/FPMIN;
		d = 1.0f/b;
		h=d;
		for (i=1;i<=MAXIT;i++)
		{
			a = -i*(nm1+i);
			b += 2.0f;
			d = 1.0f/(a*d+b);
			c = b+a/c;
			del = c*d;
			h *= del;
			if (fabs(del-1.0f) < EPS) {
				return h*exp(-x);
			}
		}

		return NAN;

	} else {

		ans = (nm1 != 0 ? 1.0/nm1 : -log(x)-EULER);
		fact=1.0;

		for (i=1;i<=MAXIT;i++)
		{
			fact *= -x/i;

			if (i != nm1) {
				del = -fact/(i-nm1);
			} else {
				psi = -EULER;

				for (ii=1;ii<=nm1;ii++)
				{
					psi += 1.0/ii;
				}
				del=fact*(-log(x)+psi);
			}
			ans += del;

			if (fabs(del) < fabs(ans)*EPS) {
				return ans;
			}
		}
		return NAN;
	}
	return ans;
}

void
destroyLogMMSE(LogMMSE** logMMSE)
{
	if((*logMMSE) != NULL){
		int i, j;
		if((*logMMSE)->gainTable != NULL){
			for(i=0;i<(*logMMSE)->classes;i++){
				for(j=0;j<60;j++){
					free((*logMMSE)->gainTable[i][j]);
				}
				free((*logMMSE)->gainTable[i]);
			}
			free((*logMMSE)->gainTable);
			(*logMMSE)->gainTable = NULL;
		}
		if((*logMMSE)->gammak != NULL){
			free((*logMMSE)->gammak);
			(*logMMSE)->gammak = NULL;
		}
		if((*logMMSE)->noiseMu2 != NULL){
			free((*logMMSE)->noiseMu2);
			(*logMMSE)->noiseMu2 = NULL;
		}
		if((*logMMSE)->previousXk != NULL){
			free((*logMMSE)->previousXk);
			(*logMMSE)->previousXk = NULL;
		}
		if((*logMMSE)->previousRk != NULL){
			free((*logMMSE)->previousRk);
			(*logMMSE)->previousRk = NULL;
		}
		if((*logMMSE)->smoothPost != NULL){
			free((*logMMSE)->smoothPost);
			(*logMMSE)->smoothPost = NULL;
		}
		if((*logMMSE)->hw != NULL){
			free((*logMMSE)->hw);
			(*logMMSE)->hw = NULL;
		}
		if((*logMMSE)->safetyNetP != NULL){
			for(i=0;i<(*logMMSE)->nFFT;i++){
				free((*logMMSE)->safetyNetP[i]);
			}
			free((*logMMSE)->safetyNetP);
			(*logMMSE)->safetyNetP = NULL;
		}
		if((*logMMSE)->safetyNetPMinimum != NULL){
			free((*logMMSE)->safetyNetPMinimum);
			(*logMMSE)->safetyNetPMinimum = NULL;
		}
		if((*logMMSE)->speechPresenceProbability != NULL){
			free((*logMMSE)->speechPresenceProbability);
			(*logMMSE)->speechPresenceProbability = NULL;
		}
		free(*logMMSE);
		*logMMSE = NULL;
	}
}
