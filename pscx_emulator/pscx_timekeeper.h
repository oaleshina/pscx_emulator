#pragma once

#include <cstdint>

using Cycles = uint64_t;

// List of all peripherals requiring a TimeSheet. The value of the
// enum is used as the index in the table.
enum Peripheral
{
	// Graphics Processing Unit
	PERIPHERAL_GPU = 0,
	PERIPHERAL_TIMER0,
	PERIPHERAL_TIMER1,
	PERIPHERAL_TIMER2
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
	TimeSheet m_timesheets[4];
};

// Fixed point representation of a cycle counter used to store non-integer cycle counts.
// It is required because the CPU and GPU clocks have a non-integer ratio.
struct FracCycles
{
	FracCycles(Cycles cycles) : m_cycles(cycles) {}

	static FracCycles fromFp(Cycles value)
	{
		return FracCycles(value);
	}

	static FracCycles fromF32(float value)
	{
		float precision = (float)(1 << FracCycles::fracBits());
		return (FracCycles)FracCycles(value * precision);
	}

	static FracCycles fromCycles(Cycles value)
	{
		return FracCycles(value << FracCycles::fracBits());
	}

	// Return the raw fixed point value
	Cycles getFp() const { return m_cycles; }

	// Return the number of fractional bits in the fixed point
	// representation
	static Cycles fracBits()
	{
		return 16;
	}

	FracCycles add(FracCycles value)
	{
		return FracCycles(getFp() + value.getFp());
	}

	FracCycles multiply(FracCycles mul)
	{
		Cycles cycles = getFp() * mul.getFp();

		// The shift amount is doubled during the multiplication so we
		// have to shift it back to its normal position.
		return FracCycles(cycles >> FracCycles::fracBits());
	}

	FracCycles divide(FracCycles denominator)
	{
		// In order not to lose precision we must shift the numerator
		// once more *before* the division. Otherwise the division of
		// the two shifted value would only give us the integer part of the result.
		Cycles numerator = getFp() << FracCycles::fracBits();

		return FracCycles(numerator / denominator.getFp());
	}

private:
	Cycles m_cycles;
};
