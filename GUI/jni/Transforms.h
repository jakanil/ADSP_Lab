#ifndef TRANSFORMS_H
#define TRANSFORMS_H
#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <math.h>

typedef struct Transform {
		int points;
		int windowSize;
		float* real;
		float* imaginary;
		float* power;
		float* sine;
		float* cosine;
		float* window;
} Transform;

Transform* newTransform(int windowSize);
void ForwardFFT(Transform* fft, float* real);
void InverseFFT(Transform* fft);
void destroyTransform(Transform** transform);

#endif
