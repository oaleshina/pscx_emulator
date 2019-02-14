#include "pscx_timekeeper.h"
#include <cstdlib>

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

Cycles TimeSheet::getNextSync() const
{
	return m_nextSync;
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
	Cycles date = m_now + delta;
	m_timesheets[who].setNextSync(date);

	if (date < m_nextSync)
		m_nextSync = date;
}

void TimeKeeper::noSyncNeeded(Peripheral who)
{
	// Insted of sisabling the sync completely we can just use a distant date.
	// Peripheral's syncs should be idempotent.
	m_timesheets[who].setNextSync(ULLONG_MAX);
}

bool TimeKeeper::syncPending() const
{
	return m_nextSync <= m_now;
}

bool TimeKeeper::needsSync(Peripheral who) const
{
	return m_timesheets[who].needsSync(m_now);
}

void TimeKeeper::updateSyncPending()
{
	Cycles minNextSync = ULLONG_MAX;
	for (size_t i = 0; i < _countof(m_timesheets); ++i)
	{
		Cycles nextSync = m_timesheets[i].getNextSync();
		if (minNextSync > nextSync) minNextSync = nextSync;
	}
}
