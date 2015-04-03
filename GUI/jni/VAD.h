#ifndef VAD_H
#define VAD_H
//#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <math.h>
#include <FIR.h>

#define EPSQB 0.001f
#define ALPHA 0.975f
#define ADS 0.65f

#define DECOMPOSITION_FILTER_LENGTH 12

typedef struct VoiceDetector {
		int numFrames;
		int frameSize;
		float* DsBuffer;
		float* DsBufferSorted;
		int DsBufferIndex;
		float previousTqb;
		float previousDs;
		int noiseCount;
		int vadDecision;
		FIRFilter* low;
		FIRFilter* high;
		void (*doVAD)(struct VoiceDetector* vad, float* input);
} VoiceDetector;

VoiceDetector* newVAD(int frequency, int framesize);
void destroyVAD(VoiceDetector** vad);

#endif
