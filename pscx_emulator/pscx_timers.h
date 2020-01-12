#pragma once

#include <cassert>

#include "pscx_common.h"
#include "pscx_timekeeper.h"
#include "pscx_interrupts.h"
//#include "pscx_gpu.h"

struct Gpu;

// Various synchronization modes when the timer is not in free-run
enum SyncTimer
{
	// For timer 1/2: Pause during H/VBlank. For timer 3: Stop counter
	SYNC_TIMER_PAUSE = 0,
	// For timer 1/2: Reset counter at H/VBlank. For timer 3: Free run
	SYNC_TIMER_RESET = 1,
	// For timer 1/2: Reset counter at H/VBlank and pause outside of it.
	// For timer 3: Free run
	SYNC_TIMER_RESET_AND_PAUSE = 2,
	// For timer 1/2: Wait for H/VBlank and then free-run. For timer 3: Stop counter
	SYNC_TIMER_WAIT_FOR_SYNC = 3
};

// The four posiible clock sources for the timers
enum Clock
{
	// The CPU clock at ~33.87MHz
	CLOCK_SOURCE_SYS_CLOCK,
	// The CPU clock divided by 8 (~4.23MHz)
	CLOCK_SOURCE_SYS_CLOCK_DIV_8,
	// The GPU's dotclock (depends on hardware, around 53MHz)
	CLOCK_SOURCE_GPU_DOT_CLOCK,
	// The GPU's HSync signal (depends on hardware and video timings)
	CLOCK_SOURCE_GPU_HSYNC
};

// Returns true if the clock comes from the GPU
bool needsGpu(Clock clock);

// Clock source is stored on two bits. The meaning of those bits
// depends on the timer instance
struct ClockSource
{
	ClockSource(uint8_t clockSource) : m_clockSource(clockSource) {}

	static ClockSource fromField(uint16_t field)
	{
		assert(("Invalid clock source", !(field & ~3)));
		return ClockSource((uint8_t)field);
	}

	Clock clock(Peripheral instance) const;
	uint8_t getClockSource() const;

private:
	uint8_t m_clockSource;
};

struct Timer
{
	Timer(Peripheral instance) :
		m_instance(instance),
		m_counter(0x0),
		m_target(0x0),
		m_useSync(false),
		m_sync(SyncTimer::SYNC_TIMER_PAUSE),
		m_targetWrap(false),
		m_targetIrq(false),
		m_wrapIrq(false),
		m_repeatIrq(false),
		m_negateIrq(false),
		m_clockSource(ClockSource::fromField(0x0)),
		m_targetReached(false),
		m_overflowReached(false),
		m_period(FracCycles::fromCycles(0x1)),
		m_phase(FracCycles::fromCycles(0x0)),
		m_interrupt(false)
	{}

	// Recomputes the entire timer's internal state. Must be called
	// when the timer's config changes or when the timer relies on
	// the GPU's video timings and those timings change.

	// If the GPU is needed for the timings it must be synchronized
	// before this function is called.
	void reconfigure(const Gpu& gpu, TimeKeeper& timeKeeper);

	// Synchronize this timer.
	void sync(TimeKeeper& timeKeeper, InterruptState& irqState);

	void predictNextSync(TimeKeeper& timeKeeper);

	// Return true if the timer relies on the GPU for the clock
	// source or synchronization
	bool needsGpu();

	uint16_t getMode();

	// Set the value of the mode register
	void setMode(uint16_t value);

	uint16_t getTarget() const;
	void setTarget(uint16_t value);

	uint16_t getCounter() const;
	void setCounter(uint16_t value);

private:
	// Timer instance (Timer0, 1 or 2)
	Peripheral m_instance;

	// Counter value
	uint16_t m_counter;

	// Counter target
	uint16_t m_target;

	// If true we synchronize the timer with an external signal
	bool m_useSync;

	// The synchronization mode when "freeRun" is false. Each one of three timers
	// interprets this mode differently.
	SyncTimer m_sync;

	// If true the counter is reset when it reaches the "target" value.
	// Otherwise let it count all the way to 0xffff and wrap around.
	bool m_targetWrap;

	// Raise interrupt when the counter reasches the "target"
	bool m_targetIrq;

	// Raise interrupt when the counter passes 0xffff and wraps around
	bool m_wrapIrq;

	// If true the interrupt is automatically cleared and will re-trigger when one
	// of the interrupt conditions occurs again.
	bool m_repeatIrq;

	// If true the irq signal is inverted each time an interrupt
	// condition is reached instead of a single pulse.
	bool m_negateIrq;

	// Clock source (2 bits). Each time can either use the CPU
	// SysClock or an alternative clock source.
	ClockSource m_clockSource;

	// True if the target has been reached since the last read
	bool m_targetReached;

	// True if the counter reached 0xffff and overflowed since the last read
	bool m_overflowReached;

	// Period of a counter tick. Stored as a fractional cycle count
	// since the GPU can be used as a source.
	FracCycles m_period;

	// Current position within a period of a counter tick
	FracCycles m_phase;

	// True if interrupt signal is active
	bool m_interrupt;
};

struct Timers
{
	Timers() : m_timers { Timer(Peripheral::PERIPHERAL_TIMER0), 
						  Timer(Peripheral::PERIPHERAL_TIMER1),
						  Timer(Peripheral::PERIPHERAL_TIMER2) }
	{}

	template<typename T>
	void store(TimeKeeper& timeKeeper, InterruptState& irqState, 
		Gpu& gpu, uint32_t offset, T value);

	template<typename T>
	T load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset);

	// Called by the GPU when the video timings change since it can
	// affect the timers that use them.
	void videoTimingsChanged(TimeKeeper& timeKeeper, InterruptState& irqState, Gpu& gpu);

	void sync(TimeKeeper& timeKeeper, InterruptState& irqState);

private:
	// The three timers. They're mostly identical except that they
	// can each select a unique clock source besides the regular system clock:
	// Timer 0: GPU pixel clock
	// Timer 1: GPU horizontal blanking
	// Timer 2: System clock / 8
	Timer m_timers[3];
};
