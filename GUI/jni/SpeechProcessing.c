#include <jni.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "Timer.h"
#include "Transforms.h"
#include "MFCC.h"
#include "VAD.h"
#include "Classifier.h"
#include "LogMMSE.h"
#include "Synthesis.h"

typedef struct Variables {
	Classifier* classifier;
	Timer* timer;
	VoiceDetector* vad;
	Transform* fft;
	MelFCC* mfcc;
	LogMMSE* logMMSE;
	Synthesis* synthesis;
	float* inputBuffer;
	float* outputBuffer;
	short* originalInput;
	int frequency;
	int stepSize;
	int windowSize;
	int overlap;
	int class;
} Variables;

static jlong
initialize(JNIEnv* env, jobject thiz, jobject assetManager, jint frequency, jint stepsize, jint windowsize, jint decisionBufferLength)
{

	#ifdef LOGCAT_DEBUG
		__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "Initialization: Sampling Frequency %d, Step Size %d, Window Size %d", frequency, stepsize, windowsize);
	#endif

	Variables* inParam = (Variables*) malloc(sizeof(Variables));
	inParam->classifier = newClassifier(AAssetManager_fromJava(env, assetManager), decisionBufferLength);
	inParam->timer = newTimer();
	inParam->vad = newVAD(frequency, stepsize);
	inParam->fft = newTransform(windowsize);
	inParam->mfcc = newMFCC(frequency, inParam->fft->points);
	inParam->logMMSE = newLogMMSE(3, inParam->fft->points, frequency, (windowsize-stepsize));

	#ifdef LOGCAT_DEBUG
		__android_log_print(ANDROID_LOG_ERROR, "Speech Processing", "Initialization: FFT Points %d", inParam->logMMSE->nFFT);
	#endif

	inParam->synthesis = newSynthesis(stepsize, windowsize, inParam->fft->window);
	inParam->overlap = windowsize - stepsize;
	inParam->frequency = frequency;
	inParam->stepSize = stepsize;
	inParam->windowSize = windowsize;
	inParam->inputBuffer = (float*)calloc(windowsize,sizeof(float));
	inParam->outputBuffer = (float*)malloc(stepsize*sizeof(float));
	inParam->originalInput = (short*)malloc(stepsize*sizeof(short));

	return (jlong)inParam;
}

static void
compute(JNIEnv *env, jobject thiz,  jlong memoryPointer, jshortArray input)
{
	Variables* inParam = (Variables*) memoryPointer;
	startTimer(inParam->timer);

	short *_in = (*env)->GetShortArrayElements(env, input, NULL);

	int i;
	for(i=0; i<inParam->overlap; i++)
	{
		inParam->inputBuffer[i] = inParam->inputBuffer[inParam->stepSize + i];
	}

	for (i=0; i<inParam->stepSize; i++)
	{
		inParam->originalInput[i] = _in[i];
		inParam->inputBuffer[inParam->overlap + i] = _in[i]/32768.0f;
	}

	(*env)->ReleaseShortArrayElements(env, input, _in, 0);

	inParam->vad->doVAD(inParam->vad, &(inParam->inputBuffer[inParam->overlap]));
	ForwardFFT(inParam->fft, inParam->inputBuffer);

	if(inParam->vad->vadDecision != 1) {
		computeMFCC(inParam->mfcc, inParam->fft->power);
		inParam->classifier->doClassification(inParam->classifier, inParam->mfcc->MelCoeffOut);
	}

	if((inParam->class < 0) || (inParam->class >= inParam->logMMSE->classes)) {
		inParam->logMMSE->suppress(inParam->logMMSE, 0, inParam->fft->power);
	} else {
		inParam->logMMSE->suppress(inParam->logMMSE, inParam->class, inParam->fft->power);
	}

	for (i=0; i<inParam->fft->points; i++)
	{
		inParam->fft->real[i] *= inParam->logMMSE->hw[i];
		inParam->fft->imaginary[i] *= inParam->logMMSE->hw[i];
	}

	InverseFFT(inParam->fft);

	inParam->synthesis->doSynthesis(inParam->synthesis, inParam->fft->real);

	stopTimer(inParam->timer);
}

static jfloat
getTime(JNIEnv* env, jobject thiz, jlong memoryPointer)
{
	Variables* inParam = (Variables*) memoryPointer;
	return getTimerMS(inParam->timer);
}

static void
getOutput(JNIEnv* env, jobject thiz, jlong memoryPointer, jint outputSelect, jshortArray output)
{
	Variables* inParam = (Variables*) memoryPointer;

	short *_output = (*env)->GetShortArrayElements(env, output, NULL);

	if(outputSelect == 0) { //Case 1 - Original input signal
		int i;
		for(i=0;i<inParam->stepSize;i++)
		{
			_output[i] = inParam->originalInput[i];
		}

	} else {				//Case 2 - Processed output signal
		int i;
		for(i=0;i<inParam->stepSize;i++)
		{
			_output[i] = (short)(inParam->synthesis->output[i]*32768.0f);
		}
	}

	(*env)->ReleaseShortArrayElements(env, output, _output, 0);
}

static jfloatArray
copyArray(JNIEnv* env, float* array, int length)
{
	jfloatArray output = (*env)->NewFloatArray(env, length);
	float *_output = (*env)->GetFloatArrayElements(env, output, NULL);

	int i;
	for (i=0; i<length;i++)
	{
		_output[i] = array[i];
	}

	(*env)->ReleaseFloatArrayElements(env, output, _output, 0);

	return output;
}

static jfloatArray
getDebug(JNIEnv* env, jobject thiz, jlong memoryPointer, jint debugSelect)
{
	Variables* inParam = (Variables*) memoryPointer;

	jfloatArray debugOutput = NULL;

	if(debugSelect == 0) {

		//Test Case 0 - inputBuffer contents
		debugOutput = copyArray(env, inParam->inputBuffer, inParam->windowSize);

	} else if(debugSelect == 1) {

		//Test Case 1 - input frame
		debugOutput = copyArray(env, &(inParam->inputBuffer[inParam->overlap]), inParam->stepSize);

	}  else if (debugSelect == 2) {

		//Test Case 2 - Synthesis Output
		debugOutput = copyArray(env, inParam->synthesis->output, inParam->stepSize);

	} else if (debugSelect == 3) {

		//Test Case 3 - FFT Power Spectrum
		debugOutput = copyArray(env, inParam->fft->power, inParam->fft->points);

	} else if (debugSelect == 4) {

		//Test Case 4 - FFT Real Portion
		debugOutput = copyArray(env, inParam->fft->real, inParam->fft->points);

	} else if (debugSelect == 5) {

		//Test Case 5 - FFT Imaginary Portion
		debugOutput = copyArray(env, inParam->fft->imaginary, inParam->fft->points);

	} else if (debugSelect == 6) {

		//Test Case 6 - MFCC Filtered FFT Output
		debugOutput = copyArray(env, inParam->mfcc->MelFFT, inParam->mfcc->NFilters);

	} else if (debugSelect == 7) {

		//Test Case 7 - MFCC Coefficient Output
		debugOutput = copyArray(env, inParam->mfcc->MelCoeffOut, CEPSTRAL_COEFFICIENTS);

	} else if (debugSelect == 8) {

		//Test Case 8 - VAD High Filtered
		debugOutput = copyArray(env, inParam->vad->high->result, inParam->stepSize);

	} else if (debugSelect == 9) {

		//Test Case 9 - VAD Low Filtered
		debugOutput = copyArray(env, inParam->vad->low->result, inParam->stepSize);

	} else if (debugSelect == 10) {

		//Test Case 10 - Ds Buffer
		debugOutput = copyArray(env, inParam->vad->DsBuffer, (inParam->frequency/inParam->stepSize));

	} else if (debugSelect == 11) {

		//Test Case 11 - Ds Buffer Sorted
		debugOutput = copyArray(env, inParam->vad->DsBufferSorted, (inParam->frequency/inParam->stepSize));

	} else if (debugSelect == 12) {

		//Test Case 12 - VAD Outputs (Decision, Dc, Tqb)

		debugOutput = (*env)->NewFloatArray(env, 3);
		float *_debugOutput = (*env)->GetFloatArrayElements(env, debugOutput, NULL);

		_debugOutput[0] = inParam->vad->vadDecision;
		_debugOutput[1] = inParam->vad->previousDs;
		_debugOutput[2] = inParam->vad->previousTqb;

		(*env)->ReleaseFloatArrayElements(env, debugOutput, _debugOutput, 0);

	} else if (debugSelect == 13) {

		//Test Case 13 - Classifier Outputs (Decision, (Class count, Probability))

		debugOutput = (*env)->NewFloatArray(env, 1+2*inParam->classifier->classes);
		float *_debugOutput = (*env)->GetFloatArrayElements(env, debugOutput, NULL);

		_debugOutput[0] = inParam->classifier->classDecision;

		int i;
		for(i=1;i<=inParam->classifier->classes;i++)
		{
			_debugOutput[i] = inParam->classifier->classDecisionCount[i-1];
		}

		for(i=1;i<=inParam->classifier->classes;i++)
		{
			_debugOutput[i+inParam->classifier->classes] = inParam->classifier->gmmClassifiers[i-1]->probability;
		}

		(*env)->ReleaseFloatArrayElements(env, debugOutput, _debugOutput, 0);

	} else if (debugSelect == 14) {

		//Test Case 14 - NoiseMu2
		debugOutput = copyArray(env, inParam->logMMSE->noiseMu2, inParam->logMMSE->nFFT);

	} else if (debugSelect == 15) {

		//Test Case 15 - gammak
		debugOutput = copyArray(env, inParam->logMMSE->gammak, inParam->logMMSE->nFFT);

	} else if (debugSelect == 16) {

		//Test Case 16 - previousRk
		debugOutput = copyArray(env, inParam->logMMSE->previousRk, inParam->logMMSE->nFFT);

	} else if (debugSelect == 17) {

		//Test Case 17 - previousXk
		debugOutput = copyArray(env, inParam->logMMSE->previousXk, inParam->logMMSE->nFFT);

	} else if (debugSelect == 18) {

		//Test Case 18 - safetyNetPMinimum
		debugOutput = copyArray(env, inParam->logMMSE->safetyNetPMinimum, inParam->logMMSE->nFFT);

	} else if (debugSelect == 19) {

		//Test Case 19 - smoothPost
		debugOutput = copyArray(env, inParam->logMMSE->smoothPost, inParam->logMMSE->nFFT);

	} else if (debugSelect == 20) {

		//Test Case 20 - speechPresenceProbability
		debugOutput = copyArray(env, inParam->logMMSE->speechPresenceProbability, inParam->logMMSE->nFFT);

	} else if (debugSelect == 21) {

		//Test Case 21 - hw
		debugOutput = copyArray(env, inParam->logMMSE->hw, inParam->logMMSE->nFFT);

	}

	return debugOutput;
}

static void
finish(JNIEnv* env, jobject thiz, jlong memoryPointer)
{
	Variables* inParam = (Variables*) memoryPointer;
	//cleanup memory
	if(inParam != NULL){

		#ifdef LOGCAT_DEBUG
			tellTimerTime(inParam->timer);
		#endif

		destroyClassifier(&(inParam->classifier));
		destroyTimer(&(inParam->timer));
		destroyVAD(&(inParam->vad));
		destroyTransform(&(inParam->fft));
		destroyMFCC(&(inParam->mfcc));
		destroyLogMMSE(&(inParam->logMMSE));
		destroySynthesis(&(inParam->synthesis));
		if(inParam->inputBuffer != NULL){
			free(inParam->inputBuffer);
			inParam->inputBuffer = NULL;
		}
		if(inParam->outputBuffer != NULL){
			free(inParam->outputBuffer);
			inParam->outputBuffer = NULL;
		}
		if(inParam->originalInput != NULL){
			free(inParam->originalInput);
			inParam->originalInput = NULL;
		}
		free(inParam);
		inParam = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// JNI Setup - Functions and OnLoad
////////////////////////////////////////////////////////////////////////////////////////////

static JNINativeMethod nativeMethods[] =
	{//		Name							Signature											Pointer
			{"compute", 					"(J[S)V",											(void *)&compute				},
			{"initialize",					"(Landroid/content/res/AssetManager;IIII)J",		(void *)&initialize				},
			{"finish",						"(J)V",												(void *)&finish					},
			{"getTime",						"(J)F",												(void *)&getTime				},
			{"getOutput",					"(JI[S)V",											(void *)&getOutput				},
			{"getDebug",					"(JI)[F",											(void *)&getDebug				}
	};

jint
JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env;
	jint result;
	//get a hook to the environment
	result = (*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_6);
	if (result == JNI_OK) {
		//find the java class to hook the native methods to
		jclass filters = (*env)->FindClass(env, "com/dsp/speechpipeline/SpeechProcessing");
		if (filters != NULL) {
			result = (*env)->RegisterNatives(env, filters, nativeMethods, sizeof(nativeMethods)/sizeof(nativeMethods[0]));
			(*env)->DeleteLocalRef(env, filters);
			if(result == JNI_OK){
				return JNI_VERSION_1_6;
			} else {
				//something went wrong with the method registration
				return JNI_ERR;
			}
		} else {
			//class wasn't found
			return JNI_ERR;
		}
	} else {
		//could not get environment
		return JNI_ERR;
	}
}
