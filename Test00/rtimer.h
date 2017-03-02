#pragma once

class RTimer  
{
public:
	RTimer();
	virtual ~RTimer();

	void	StartTimer();
	void	StopTimer();
	double	GetElapsedTime();
	double	GetElapsedTime(__int64 lliElapsedTime);
	__int64	GetTime();

private:
	double	m_dTicksPerSecond;
	__int64	m_lliStartTick;
};
