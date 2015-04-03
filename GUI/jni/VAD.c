#include "VAD.h"

void initVAD(VoiceDetector* vad, float* input);
void VAD(VoiceDetector* vad, float* input);

static float LOWDECOMPOSITION[] = {0.0154041093270274, 0.00349071208421747, -0.117990111148191, -0.0483117425856330,
					 0.491055941926747, 0.787641141030194, 0.337929421727622, -0.0726375227864625,
					 -0.0210602925123006, 0.0447249017706658, 0.00176771186424280, -0.00780070832503415};

static float HIGHDECOMPOSITION[] = {0.00780070832503415, 0.00176771186424280, -0.0447249017706658, -0.0210602925123006,
					  0.0726375227864625, 0.337929421727622, -0.787641141030194, 0.491055941926747,
					  0.0483117425856330, -0.117990111148191, -0.00349071208421747, 0.0154041093270274};

VoiceDetector*
newVAD(int frequency, int framesize)
{
	VoiceDetector* newVAD = (VoiceDetector*)malloc(sizeof(VoiceDetector));

	newVAD->frameSize = framesize;
	newVAD->numFrames = (frequency/framesize);					//1 second window
	newVAD->DsBuffer = (float*)calloc((frequency/framesize),sizeof(float));
	newVAD->DsBufferSorted = (float*)calloc((frequency/framesize),sizeof(float));
	newVAD->DsBufferIndex = 0;
	newVAD->previousTqb = 0.0f;
	newVAD->previousDs = 0.0f;
	newVAD->noiseCount = 0;
	newVAD->low = newFIR(framesize, DECOMPOSITION_FILTER_LENGTH, LOWDECOMPOSITION);
	newVAD->high = newFIR(framesize, DECOMPOSITION_FILTER_LENGTH, HIGHDECOMPOSITION);
	newVAD->doVAD = initVAD;
	return newVAD;
}

int
compareFloat(const void * a, const void * b)
{
	if (*(float*)a <  *(float*)b){
		return -1;
	}
	if (*(float*)a > *(float*)b){
		return 1;
	}
	return 0;
}


void
initVAD(VoiceDetector* vad, float* input)
{
	int i;
	float Px = 0.0f;
	float Pl = 0.0f;
	float Ph = 0.0f;
	float Ds = 0.0f;

	for(i=0; i<vad->frameSize;i++)
	{
		Px += input[i]*input[i];
	}

	computeFIR(vad->low, input);
	computeFIR(vad->high, input);

	for(i=0; i<(vad->frameSize/2);i++)
	{
		Pl += vad->low->result[2*i]*vad->low->result[2*i];
		Ph += vad->high->result[2*i]*vad->high->result[2*i];
	}

	Ds = fabs(Pl-Ph)/(vad->frameSize/2);
	Ds = Ds*(0.5 + (16/log(2.0))*log(1+2*Px));
	Ds = (1.0-exp(-2*Ds))/(1.0+exp(-2*Ds));

	vad->previousDs = Ds;
	vad->previousTqb = Ds;
	vad->DsBuffer[(vad->DsBufferIndex)++] = Ds;

	//set initial decision to Voice
	vad->vadDecision = 1;

	//assume first frame is noise
	vad->noiseCount++;

	vad->doVAD = VAD;
}

void
VAD(VoiceDetector* vad, float* input)
{
	int i;
	float Px = 0.0f;
	float Pl = 0.0f;
	float Ph = 0.0f;
	float Ds = 0.0f;

	for(i=0; i<vad->frameSize;i++)
	{
		Px += input[i]*input[i];
	}

	computeFIR(vad->low, input);
	computeFIR(vad->high, input);

	for(i=0; i<(vad->frameSize/2);i++)
	{
		Pl += vad->low->result[2*i]*vad->low->result[2*i];
		Ph += vad->high->result[2*i]*vad->high->result[2*i];
	}

	Ds = fabs(Pl-Ph)/(vad->frameSize/2);
	Ds = Ds*(0.5 + (16/log(2.0))*log(1+2*Px));
	Ds = (1.0-exp(-2*Ds))/(1.0+exp(-2*Ds));
	Ds = Ds + vad->previousDs*0.65f;

	vad->previousDs = Ds;

	vad->DsBuffer[(vad->DsBufferIndex)++] = Ds;
	if((vad->DsBufferIndex)>=vad->numFrames) {
		vad->DsBufferIndex = 0;
	}

	for(i=0; i<vad->numFrames;i++)
	{
		vad->DsBufferSorted[i] = vad->DsBuffer[i];
	}

	qsort(vad->DsBufferSorted, vad->numFrames, sizeof(float), compareFloat);

	int qb = 4;
	while(((vad->DsBufferSorted[qb]-vad->DsBufferSorted[qb-4]) < EPSQB) && (qb < (vad->numFrames-1)))
	{
		qb++;
	}

	vad->previousTqb = ALPHA*vad->previousTqb + (1.0f-ALPHA)*vad->DsBufferSorted[qb];

	if(Ds > (vad->previousTqb)) {
		vad->noiseCount = 0;
		vad->vadDecision = 1;
	} else {
		if(vad->noiseCount > (vad->numFrames/5)) {//guard time = 200ms
			vad->vadDecision = 0;
		} else {
			vad->vadDecision = 1;
			vad->noiseCount++;
		}
	}
}

void
destroyVAD(VoiceDetector** vad)
{
	if(*vad != NULL){
		if((*vad)->DsBuffer != NULL){
			free((*vad)->DsBuffer);
			(*vad)->DsBuffer = NULL;
		}
		if((*vad)->DsBufferSorted != NULL){
			free((*vad)->DsBufferSorted);
			(*vad)->DsBufferSorted = NULL;
		}
		destroyFIR(&((*vad)->low));
		destroyFIR(&((*vad)->high));
		free(*vad);
		*vad = NULL;
	}
}
