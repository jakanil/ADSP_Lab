#include "Classifier.h"

void initClassification(Classifier* classifier, float* input);
void Classification(Classifier* classifier, float* input);

Classifier*
newClassifier(AAssetManager* assetManager, int majorityCount)
{
	Classifier* newClassifier = (Classifier*)malloc(sizeof(Classifier));
	newClassifier->majorityCount = majorityCount;
	newClassifier->decisionBufferIndex = 0;
	newClassifier->classDecision = -1;

	newClassifier->featureBuffer = (float*)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float));
	newClassifier->decisionBuffer = (int*)calloc(majorityCount,sizeof(int));

	if(assetManager != NULL) {

		#ifdef LOGCAT_DEBUG
			__android_log_print(ANDROID_LOG_ERROR, "Classifier", "Successfully got Android AssetManager!");
		#endif

		AAsset* filePointer = AAssetManager_open(assetManager, "SpeechProcessing/GMMParam.dat", AASSET_MODE_UNKNOWN);

		if(filePointer != NULL) {

			// Determine file size
			off_t size = AAsset_getLength(filePointer);

			// Allocate memory
			char* fileData = (char*)malloc((size+1)*sizeof(char));
			char* dataCopy = fileData;

			// "Null" terminate the "string"
			fileData[size] = 0;

			// Read the file contents
			AAsset_read(filePointer, fileData, size);

			//Close the file
			AAsset_close(filePointer);

			//Initialize GMM parameters from assets file data
			int classes = (int)strtof(dataCopy, &dataCopy);
			dataCopy++;

			newClassifier->classes = classes;
			newClassifier->gmmClassifiers = (GMM**)malloc(classes*sizeof(GMM*));
			newClassifier->classDecisionCount = (int*)calloc(classes,sizeof(int));

			//Sigma Inverse
			int i, j, k, l;
			for(i=0;i<classes;i++)
			{
				newClassifier->gmmClassifiers[i] = newGMM();

				for(j=0;j<NSTATES;j++)
				{
					for(k=0;k<CEPSTRAL_COEFFICIENTS;k++)
					{
						for(l=0;l<CEPSTRAL_COEFFICIENTS;l++)
						{
							newClassifier->gmmClassifiers[i]->sigmaInverse[j][k][l] = strtof(dataCopy, &dataCopy);
							dataCopy++;
							#ifdef LOGCAT_DEBUG
								__android_log_print(ANDROID_LOG_ERROR, "Classifier","Sigma[i,j,k,l] - [%d,%d,%d,%d] = %f", i, j, k, l, newClassifier->gmmClassifiers[i]->sigmaInverse[j][k][l]);
							#endif
						}
					}
				}
			}

			//Mu
			for(i=0;i<classes;i++)
			{
				for(j=0;j<NSTATES;j++)
				{
					for(k=0;k<CEPSTRAL_COEFFICIENTS;k++)
					{
						newClassifier->gmmClassifiers[i]->mu[j][k] = strtof(dataCopy, &dataCopy);
						dataCopy++;
						#ifdef LOGCAT_DEBUG
							__android_log_print(ANDROID_LOG_ERROR, "Classifier","Mu[i,j,k] - [%d,%d,%d] = %f", i, j, k, newClassifier->gmmClassifiers[i]->mu[j][k]);
						#endif
					}
				}
			}

			//Prior
			for(i=0;i<classes;i++)
			{
				for(j=0;j<NSTATES;j++)
				{
					newClassifier->gmmClassifiers[i]->prior[j] = strtof(dataCopy, &dataCopy);
					dataCopy++;
					#ifdef LOGCAT_DEBUG
						__android_log_print(ANDROID_LOG_ERROR, "Classifier","Prior[i,j] - [%d,%d] = %f", i, j, newClassifier->gmmClassifiers[i]->prior[j]);
					#endif
				}
			}

			//Mul Constant
			for(i=0;i<classes;i++)
			{
				for(j=0;j<NSTATES;j++)
				{
					newClassifier->gmmClassifiers[i]->mulConstant[j] = strtof(dataCopy, &dataCopy);
					dataCopy++;
					#ifdef LOGCAT_DEBUG
						__android_log_print(ANDROID_LOG_ERROR, "Classifier","MulConst[i,j] - [%d,%d] = %f", i, j, newClassifier->gmmClassifiers[i]->mulConstant[j]);
					#endif
				}
			}

			//Mean Feature
			newClassifier->featureMean = (float*)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float));
			for(i=0;i<CEPSTRAL_COEFFICIENTS;i++)
			{
				newClassifier->featureMean[i] = strtof(dataCopy, &dataCopy);
				dataCopy++;
				#ifdef LOGCAT_DEBUG
					__android_log_print(ANDROID_LOG_ERROR, "Classifier","Mean[i] - [%d] = %f", i, newClassifier->featureMean[i]);
				#endif
			}

			//Feature Variance
			newClassifier->featureVariance = (float*)malloc(CEPSTRAL_COEFFICIENTS*sizeof(float));
			for(i=0;i<CEPSTRAL_COEFFICIENTS;i++)
			{
				newClassifier->featureVariance[i] = strtof(dataCopy, &dataCopy);
				dataCopy++;
				#ifdef LOGCAT_DEBUG
					__android_log_print(ANDROID_LOG_ERROR, "Classifier","Var[i] - [%d] = %f", i, newClassifier->featureVariance[i]);
				#endif
			}

			free(fileData);
		}

		#ifdef LOGCAT_DEBUG
			else {
				__android_log_print(ANDROID_LOG_ERROR, "Classifier","Could not read GMM parameter file.");
			}
		#endif

	}
	newClassifier->doClassification = initClassification;
	return newClassifier;
}

void
initClassification(Classifier* classifier, float* input)
{
	int i;
	for(i=0;i<CEPSTRAL_COEFFICIENTS;i++) {
		classifier->featureBuffer[i] = (input[i]-classifier->featureMean[i])/classifier->featureVariance[i];
	}

	computeProbabilityGMM(classifier->gmmClassifiers[0], classifier->featureBuffer);

	int maximumProbabilityIndex = 0;
	for(i=1;i<classifier->classes;i++) {
		computeProbabilityGMM(classifier->gmmClassifiers[i], classifier->featureBuffer);
		if(classifier->gmmClassifiers[i]->probability>classifier->gmmClassifiers[maximumProbabilityIndex]->probability) {
			maximumProbabilityIndex = i;
		}
	}

	classifier->classDecisionCount[maximumProbabilityIndex]++;

	classifier->decisionBuffer[classifier->decisionBufferIndex++] = maximumProbabilityIndex;
	if((classifier->decisionBufferIndex)>=(classifier->majorityCount)) {
		classifier->decisionBufferIndex = 0;
		classifier->doClassification = Classification;
	}
}

void
Classification(Classifier* classifier, float* input)
{
	int i;
	for(i=0;i<CEPSTRAL_COEFFICIENTS;i++) {
		classifier->featureBuffer[i] = (input[i]-classifier->featureMean[i])/classifier->featureVariance[i];
	}

	computeProbabilityGMM(classifier->gmmClassifiers[0], classifier->featureBuffer);

	int maximumProbabilityIndex = 0;
	for(i=1;i<classifier->classes;i++) {
		computeProbabilityGMM(classifier->gmmClassifiers[i], classifier->featureBuffer);
		if(classifier->gmmClassifiers[i]->probability>classifier->gmmClassifiers[maximumProbabilityIndex]->probability) {
			maximumProbabilityIndex = i;
		}
	}

	classifier->classDecisionCount[classifier->decisionBuffer[classifier->decisionBufferIndex]]--;
	classifier->classDecisionCount[maximumProbabilityIndex]++;

	classifier->decisionBuffer[classifier->decisionBufferIndex++] = maximumProbabilityIndex;
	if((classifier->decisionBufferIndex)>=(classifier->majorityCount)) {
		classifier->decisionBufferIndex = 0;
	}

	int maxCountIndex = 0;
	for(i=1;i<classifier->classes;i++) {
		if(classifier->classDecisionCount[i]>classifier->classDecisionCount[maxCountIndex]) {
			maxCountIndex = i;
		}
	}
	classifier->classDecision = maxCountIndex;
}

void
destroyClassifier(Classifier** classifier)
{
	if(*classifier != NULL){
		int i;
		if((*classifier)->gmmClassifiers != NULL){
			for(i=0;i<(*classifier)->classes;i++){
				destroyGMM(&((*classifier)->gmmClassifiers[i]));
			}
			free((*classifier)->gmmClassifiers);
			(*classifier)->gmmClassifiers = NULL;
		}
		if((*classifier)->featureMean != NULL){
			free((*classifier)->featureMean);
			(*classifier)->featureMean = NULL;
		}
		if((*classifier)->featureVariance != NULL){
			free((*classifier)->featureVariance);
			(*classifier)->featureVariance = NULL;
		}
		if((*classifier)->featureBuffer != NULL){
			free((*classifier)->featureBuffer);
			(*classifier)->featureBuffer = NULL;
		}
		if((*classifier)->decisionBuffer != NULL){
			free((*classifier)->decisionBuffer);
			(*classifier)->decisionBuffer = NULL;
		}
		if((*classifier)->classDecisionCount != NULL){
			free((*classifier)->classDecisionCount);
			(*classifier)->classDecisionCount = NULL;
		}
		free(*classifier);
		*classifier = NULL;
	}
}
