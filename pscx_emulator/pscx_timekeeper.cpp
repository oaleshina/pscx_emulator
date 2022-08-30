#include "pscx_timekeeper.h"
#include <cstdlib>
#include <iostream>

// ********************** TimeSheet implementation **********************
Cycles TimeSheet::sync(Cycles now)
{
	Cycles delta = now - m_lastSync;
	m_lastSync = now;
	return delta;
}

Cycles TimeSheet::getNextSync() const
{
	return m_nextSync;
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
	return m_timesheets[getPeripheralCode(who)].sync(m_now);
}

void TimeKeeper::setNextSyncDelta(Peripheral who, Cycles delta)
{
	Cycles date = m_now + delta;
	m_timesheets[getPeripheralCode(who)].setNextSync(date);

	if (date < m_nextSync)
	{
		m_nextSync = date;
	}
}

void TimeKeeper::setNextSyncDeltaIfCloser(Peripheral who, Cycles delta)
{
	Cycles date = m_now + delta;
	if (m_timesheets[getPeripheralCode(who)].getNextSync() > date)
	{
		m_timesheets[getPeripheralCode(who)].setNextSync(date);
	}
}

void TimeKeeper::noSyncNeeded(Peripheral who)
{
	// Insted of sisabling the sync completely we can just use a distant date.
	// Peripheral's syncs should be idempotent.
	m_timesheets[getPeripheralCode(who)].setNextSync(ULLONG_MAX);
}

bool TimeKeeper::syncPending() const
{
	return m_nextSync <= m_now;
}

bool TimeKeeper::needsSync(Peripheral who) const
{
	return m_timesheets[getPeripheralCode(who)].needsSync(m_now);
}

void TimeKeeper::updateSyncPending()
{
	Cycles minNextSync = ULLONG_MAX;
	for (size_t i = 0; i < _countof(m_timesheets); ++i)
	{
		Cycles nextSync = m_timesheets[i].getNextSync();
		if (minNextSync > nextSync)
		{
			minNextSync = nextSync;
		}
	}
	m_nextSync = minNextSync;
}
