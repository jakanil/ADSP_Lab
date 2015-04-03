#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include <stdlib.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#ifdef LOGCAT_DEBUG
	#include <android/log.h>
#endif

#include "Constants.h"
#include "GMM.h"


typedef struct Classifier {
		GMM** gmmClassifiers;
		float* allProbabilities;
		int classes;
		int majorityCount;
		float* featureMean;
		float* featureVariance;
		float* featureBuffer;
		int* decisionBuffer;
		int decisionBufferIndex;
		int* classDecisionCount;
		int classDecision;
		void (*doClassification)(struct Classifier* classifier, float* input);
} Classifier;

Classifier* newClassifier(AAssetManager* assetManager, int majorityCount);
void destroyClassifier(Classifier** classifier);

#endif
