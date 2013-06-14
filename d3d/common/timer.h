//---------------------------------------------------------------------------------------
//
// Simple timer
//
//---------------------------------------------------------------------------------------

#ifndef _INCGUARD_TIMER_H
#define _INCGUARD_TIMER_H

#include "types.h"

class Timer
{
public:
	Timer();

	float TotalTime() const;  // in seconds
	float DeltaTime() const; // in seconds

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double m_secondsPerCount;
	double m_deltaTime;

	int64 m_baseTime;
	int64 m_pausedTime;
	int64 m_stopTime;
	int64 m_prevTime;
	int64 m_currentTime;

	bool m_stopped;
};

#endif // _INCGUARD_TIMER_H
