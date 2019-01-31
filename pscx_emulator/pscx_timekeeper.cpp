#include "pscx_timekeeper.h"

// ********************** TimeSheet implementation **********************
Cycles TimeSheet::sync(Cycles now)
{
	Cycles delta = now - m_lastSync;
	m_lastSync = now;
	return delta;
}

void TimeSheet::setNextSync(Cycles when)
{
	m_nextSync = when;
}

bool TimeSheet::needsSync(Cycles now) const
{
	return m_nextSync <= now;
}

// ********************** TimeKeeper implementation **********************
void TimeKeeper::tick(Cycles cycles)
{
	m_now += cycles;
}

Cycles TimeKeeper::sync(Peripheral who)
{
	return m_timesheets[who].sync(m_now);
}

void TimeKeeper::setNextSyncDelta(Peripheral who, Cycles delta)
{
	m_timesheets[who].setNextSync(m_now + delta);
}

bool TimeKeeper::needsSync(Peripheral who) const
{
	return m_timesheets[who].needsSync(m_now);
}
