#pragma once

#include <cstdint>

using Cycles = uint64_t;

// List of all peripherals requiring a TimeSheet. The value of the
// enum is used as the index in the table.
enum Peripheral
{
	// Graphics Processing Unit
	PERIPHERAL_GPU = 0
};

// Struct used to keep track of individual peripherals
struct TimeSheet
{
	TimeSheet() : 
		m_lastSync(0x0),
		// We force a synchtonization at startup to initialize
		// everything
		m_nextSync(0x0)
	{}

	// Forward the time sheet to the current date and return the elapsed
	// time sicne the last sync.
	Cycles sync(Cycles now);

	void setNextSync(Cycles when);
	bool needsSync(Cycles now) const;

private:
	// Date of the last synchronization
	Cycles m_lastSync;
	// Date of the next "forced" sync
	Cycles m_nextSync;
};

// Struct keeping track of the various peripheral's
// emulation advancement
struct TimeKeeper
{
	TimeKeeper() : m_now(0x0) {}

	void tick(Cycles cycles);

	// Synchronize the timesheet for the given peripheral and return
	// the elapsed time since the last sync.
	Cycles sync(Peripheral who);

	void setNextSyncDelta(Peripheral who, Cycles delta);
	bool needsSync(Peripheral who) const;

private:
	// Counter keeping track of the current date. Unit is a period of
	// the CPU clock at 33.8685MHz
	Cycles m_now;
	// Time sheets for keeping track of the various peripherals
	TimeSheet m_timesheets[1];
};
