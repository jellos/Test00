#include "stdafx.h"
#include "rtimer.h"
#include "windows.h"

RTimer::RTimer()
{
	LARGE_INTEGER lliTicksPerSecond;

	QueryPerformanceFrequency(&lliTicksPerSecond);
	m_dTicksPerSecond	= (double)lliTicksPerSecond.QuadPart;

	QueryPerformanceCounter((LARGE_INTEGER *)&m_lliStartTick);
}

RTimer::~RTimer()
{
}

double RTimer::GetElapsedTime()
{
	if (m_lliStartTick == 0)
		return 0;

	LARGE_INTEGER	lliTick;
	QueryPerformanceCounter(&lliTick);

	return (double)(lliTick.QuadPart - m_lliStartTick) / m_dTicksPerSecond;
}

double RTimer::GetElapsedTime(__int64 lliStartTick)
{
	LARGE_INTEGER	lliTick;
	QueryPerformanceCounter(&lliTick);

	return (double)(lliTick.QuadPart - lliStartTick) / m_dTicksPerSecond;
}

void RTimer::StartTimer()
{
	QueryPerformanceCounter((LARGE_INTEGER *)&m_lliStartTick);
}

__int64 RTimer::GetTime()
{
	LARGE_INTEGER lliTick;
	QueryPerformanceCounter(&lliTick);

	return lliTick.QuadPart;
}

void RTimer::StopTimer()
{
	m_lliStartTick = 0;
}
