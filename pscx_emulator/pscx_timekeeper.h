#pragma once

#include <cstdint>
#include <climits>
#include <cassert>

using Cycles = uint64_t;

// List of all peripherals requiring a TimeSheet. The value of the
// enum is used as the index in the table.
enum class Peripheral
{
	// Graphics Processing Unit
	PERIPHERAL_GPU = 0,
	PERIPHERAL_TIMER0,
	PERIPHERAL_TIMER1,
	PERIPHERAL_TIMER2,
	// Gamepad/Memory Card controller
	PERIPHERAL_PAD_MEMCARD,
	// CD-ROM controller
	PERIPHERAL_CDROM
};

inline uint64_t getPeripheralCode(Peripheral peripheral)
{
	uint64_t peripheralCode{ 0 };
	switch (peripheral)
	{
	case Peripheral::PERIPHERAL_GPU:
	{
		peripheralCode = 0;
		break;
	}
	case Peripheral::PERIPHERAL_TIMER0:
	{
		peripheralCode = 1;
		break;
	}
	case Peripheral::PERIPHERAL_TIMER1:
	{
		peripheralCode = 2;
		break;
	}
	case Peripheral::PERIPHERAL_TIMER2:
	{
		peripheralCode = 3;
		break;
	}
	case Peripheral::PERIPHERAL_PAD_MEMCARD:
	{
		peripheralCode = 4;
		break;
	}
	case Peripheral::PERIPHERAL_CDROM:
	{
		peripheralCode = 5;
		break;
	}
	default:
	{
		assert(("Unknown Peripheral", false));
	}
	}
	return peripheralCode;
}

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

	Cycles getNextSync() const;
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
	TimeKeeper() : m_now(0x0), m_nextSync(ULLONG_MAX) {}

	void tick(Cycles cycles);

	// Synchronize the timesheet for the given peripheral and return
	// the elapsed time since the last sync.
	Cycles sync(Peripheral who);

	void setNextSyncDelta(Peripheral who, Cycles delta);
	// Set next sync only if it's closer than what's already configured.
	void setNextSyncDeltaIfCloser(Peripheral who, Cycles delta);

	// Called by a peripheral when there's no asynchronous event scheduled.
	void noSyncNeeded(Peripheral who);

	bool syncPending() const;
	bool needsSync(Peripheral who) const;

	void updateSyncPending();

private:
	// Counter keeping track of the current date. Unit is a period of
	// the CPU clock at 33.8685MHz
	Cycles m_now;
	// Next time a peripheral needs an update
	Cycles m_nextSync;
	// Time sheets for keeping track of the various peripherals
	TimeSheet m_timesheets[6];
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
		uint64_t precision = 1ULL << FracCycles::fracBits();
		return FracCycles(static_cast<Cycles>((double)value * precision));
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

	Cycles ceil() const
	{
		Cycles shift = FracCycles::fracBits();
		Cycles align = (1ULL << shift) - 1;
		return (m_cycles + align) >> shift;
	}

private:
	Cycles m_cycles;
};
