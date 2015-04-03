#include "GMM.h"

GMM*
newGMM()
{
	GMM* newGMM = (GMM*)malloc(sizeof(GMM));

	newGMM->sigmaInverse = (float***)malloc(NSTATES*sizeof(float**));
	int i, j;
	for(i=0;i<NSTATES;i++)
	{
		newGMM->sigmaInverse[i] = (float**)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float*));
		for(j=0;j<CEPSTRAL_COEFFICIENTS;j++)
		{
			newGMM->sigmaInverse[i][j] = (float*)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float));
		}
	}

	newGMM->mu = (float**)malloc(NSTATES*sizeof(float*));
	for(i=0;i<NSTATES;i++)
	{
		newGMM->mu[i] = (float*)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float));
	}

	newGMM->prior = (float*)malloc(NSTATES*sizeof(float));
	newGMM->mulConstant = (float*)malloc(NSTATES*sizeof(float));
	newGMM->dm = (float*)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float));

	newGMM->probability = 0.0f;

	return newGMM;
}

void
computeProbabilityGMM(GMM* gmm, float* input)
{
	gmm->probability = 0.0f;
	float temp, dmSigma;
	int i, j, k;

	for(i=0;i<NSTATES;i++)
	{
		for(j=0;j<CEPSTRAL_COEFFICIENTS;j++)
		{
			gmm->dm[j] = input[j]-gmm->mu[i][j];
		}

		temp = 0;
		for(j=0;j<CEPSTRAL_COEFFICIENTS;j++)
		{
			dmSigma = 0;
			for(k=0;k<CEPSTRAL_COEFFICIENTS;k++)
			{
				dmSigma += gmm->dm[k]*gmm->sigmaInverse[i][j][k];
			}
			temp += dmSigma*gmm->dm[j];
		}
		gmm->probability += ((float)exp(-0.5*temp))*gmm->mulConstant[i]*gmm->prior[i];
	}
}

void
destroyGMM(GMM** gmm)
{
	if(*gmm != NULL){
		int i, j;
		if((*gmm)->sigmaInverse != NULL){
			for(i=0;i<NSTATES;i++){
				for(j=0;j<CEPSTRAL_COEFFICIENTS;j++){
					free((*gmm)->sigmaInverse[i][j]);
				}
				free((*gmm)->sigmaInverse[i]);
			}
			free((*gmm)->sigmaInverse);
			(*gmm)->sigmaInverse = NULL;
		}
		if((*gmm)->mu != NULL){
			for(i=0;i<NSTATES;i++){
				free((*gmm)->mu[i]);
			}
			free((*gmm)->mu);
			(*gmm)->mu = NULL;
		}
		if((*gmm)->prior != NULL){
			free((*gmm)->prior);
			(*gmm)->prior = NULL;
		}
		if((*gmm)->mulConstant != NULL){
			free((*gmm)->mulConstant);
			(*gmm)->mulConstant = NULL;
		}
		if((*gmm)->dm != NULL){
			free((*gmm)->dm);
			(*gmm)->dm = NULL;
		}
		free(*gmm);
		*gmm = NULL;
	}
}
