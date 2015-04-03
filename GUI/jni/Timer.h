#ifndef TIMER_H
#define TIMER_H

#include <stdlib.h>
#include <time.h>
#ifdef LOGCAT_DEBUG
	#include <android/log.h>
#endif

typedef struct Timer {
		unsigned int totalRuns;
		unsigned long long totalTime;
        struct timespec startTime;
        struct timespec stopTime;
} Timer;

Timer* newTimer();

void startTimer(Timer* timer);
void stopTimer(Timer* timer);
#ifdef LOGCAT_DEBUG
	void tellTimerTime(Timer* timer);
#endif
float getTimerMS(Timer* timer);

void destroyTimer(Timer** timer);

#endif
