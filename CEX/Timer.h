#ifndef CEX_TIMER_H
#define CEX_TIMER_H

// 'borrowed' from crypto++

#include "CexDomain.h"
#ifndef CEX_HIGHRES_TIMER
#	include <time.h>
#endif

NAMESPACE_UTILITY

#ifdef CEX_HIGHRES_TIMER
	typedef uint64_t TimerWord;
#else
	typedef clock_t TimerWord;
#endif

class TimerBase
{
public:

	enum Unit 
	{ 
		SECONDS = 0, 
		MILLISECONDS, 
		MICROSECONDS, 
		NANOSECONDS 
	};

	TimerBase(Unit unit, bool stuckAtZero) 
		: 
		m_timerUnit(unit),
		m_last(0), 
		m_start(0),
		m_stuckAtZero(stuckAtZero), 
		m_started(false) 
	{
	}

	// GetCurrentTime is a macro in MSVC 6.0
	virtual TimerWord GetCurrentTimerValue() = 0;
	// this is not the resolution, just a conversion factor into seconds
	virtual TimerWord TicksPerSecond() = 0;

	void StartTimer();
	double ElapsedTimeAsDouble();
	ulong ElapsedTime();

private:

	double ConvertTo(TimerWord t, Unit unit);
	Unit m_timerUnit;
	bool m_stuckAtZero, m_started;
	TimerWord m_start, m_last;
};

// measure CPU time spent executing instructions of this thread (if supported by OS)
// This only works correctly on Windows NT or later. On Unix it reports process time, and others wall clock time.
class ThreadUserTimer : public TimerBase
{
public:

	ThreadUserTimer(Unit unit = TimerBase::SECONDS, bool stuckAtZero = false) 
		: 
		TimerBase(unit, stuckAtZero) 
	{
	}

	TimerWord GetCurrentTimerValue();
	TimerWord TicksPerSecond();
};

// high resolution timer
class Timer : public TimerBase
{
public:

	Timer(Unit unit = TimerBase::SECONDS, bool stuckAtZero = false) 
		: 
		TimerBase(unit, stuckAtZero) 
	{
	}

	TimerWord GetCurrentTimerValue();
	TimerWord TicksPerSecond();
};

NAMESPACE_UTILITYEND
#endif