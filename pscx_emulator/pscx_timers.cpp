#include <cassert>

#include "pscx_timers.h"
#include "pscx_gpu.h"

bool needsGpu(Clock clock)
{
	return clock == (Clock::CLOCK_SOURCE_GPU_DOT_CLOCK | Clock::CLOCK_SOURCE_GPU_HSYNC);
}

// ************* ClockSource implementation ******************
Clock ClockSource::clock(Peripheral instance) const
{
	// Timers 0 and 1 use values 0 or 2 for the
	// sysclock (1 and 3 for the alternative source) while timer 2
	// uses 0 and 1 for the sysclock (2 and 3 for the alternative source).
	Clock lookup[3][4] =
	{
		// Timer 0
		{
			Clock::CLOCK_SOURCE_SYS_CLOCK, Clock::CLOCK_SOURCE_GPU_DOT_CLOCK,
			Clock::CLOCK_SOURCE_SYS_CLOCK, Clock::CLOCK_SOURCE_GPU_DOT_CLOCK
		},
		// Timer 1
		{
			Clock::CLOCK_SOURCE_SYS_CLOCK, Clock::CLOCK_SOURCE_GPU_HSYNC,
			Clock::CLOCK_SOURCE_SYS_CLOCK, Clock::CLOCK_SOURCE_GPU_HSYNC
		},
		// Timer 2
		{
			Clock::CLOCK_SOURCE_SYS_CLOCK, Clock::CLOCK_SOURCE_SYS_CLOCK,
			Clock::CLOCK_SOURCE_SYS_CLOCK_DIV_8, Clock::CLOCK_SOURCE_SYS_CLOCK_DIV_8
		}
	};
	
	Clock clock;
	switch (instance)
	{
	case Peripheral::PERIPHERAL_TIMER0:
		clock = lookup[0][m_clockSource];
		break;
	case Peripheral::PERIPHERAL_TIMER1:
		clock = lookup[1][m_clockSource];
		break;
	case Peripheral::PERIPHERAL_TIMER2:
		clock = lookup[2][m_clockSource];
		break;
	}
	return clock;
}

uint8_t ClockSource::getClockSource() const
{
	return m_clockSource;
}

// ************* Timer implementation ******************
void Timer::reconfigure(Gpu gpu, TimeKeeper& timeKeeper)
{
	switch (m_clockSource.clock(m_instance))
	{
	case Clock::CLOCK_SOURCE_SYS_CLOCK:
		m_period = FracCycles::fromCycles(1);
		m_phase = FracCycles::fromCycles(0);
		break;
	case Clock::CLOCK_SOURCE_SYS_CLOCK_DIV_8:
		m_period = FracCycles::fromCycles(8);
		m_phase = FracCycles::fromCycles(0);
		break;
	case Clock::CLOCK_SOURCE_GPU_DOT_CLOCK:
		m_period = gpu.dotclockPeriod();
		m_phase = gpu.dotclockPhase();
		break;
	case Clock::CLOCK_SOURCE_GPU_HSYNC:
		m_period = gpu.hsyncPeriod();
		m_phase = gpu.hsyncPhase();
		break;
	}

	predictNextSync(timeKeeper);
}

void Timer::sync(TimeKeeper& timeKeeper, InterruptState& irqState)
{
	Cycles delta = timeKeeper.sync(m_instance);

	if (delta == 0x0)
	{
		// The interrupt code below might glitch if it's called
		// two times in a row (trigger the interrupt twice). Let's keep it clean.
		return;
	}

	FracCycles deltaFrac = FracCycles::fromCycles(delta);

	FracCycles ticks = deltaFrac.add(m_phase);

	Cycles count = ticks.getFp() / m_period.getFp();
	Cycles phase = ticks.getFp() % m_period.getFp();

	// Store the new phase
	m_phase = FracCycles::fromFp(phase);

	count += (Cycles)m_counter;

	bool targetPassed = false;
	if ((m_counter <= m_target) && (count > (Cycles)m_target))
	{
		m_targetReached = true;
		targetPassed = true;
	}

	Cycles wrap = 0x10000;
	if (m_targetWrap)
	{
		// Wrap after the target is reached, so we need to
		// add 1 to it for our modulo to work correctly later.
		wrap = (Cycles)m_target + 1;
	}

	bool overflow = false;
	if (count >= wrap)
	{
		count %= wrap;
		if (wrap == 0x10000)
		{
			m_overflowReached = true;
			overflow = true;
		}
	}

	m_counter = (uint16_t)count;
	if ((m_wrapIrq && overflow) || (m_targetIrq && targetPassed))
	{
		Interrupt interrupt;
		switch (m_instance)
		{
		case Peripheral::PERIPHERAL_TIMER0:
			interrupt = Interrupt::INTERRUPT_TIMER0;
			break;
		case Peripheral::PERIPHERAL_TIMER1:
			interrupt = Interrupt::INTERRUPT_TIMER1;
			break;
		case Peripheral::PERIPHERAL_TIMER2:
			interrupt = Interrupt::INTERRUPT_TIMER2;
		default:
			// Unreachable
			break;
		}

		assert(m_negateIrq == false, "Unhandled negate IRQ!");

		// Pulse interrupt
		irqState.raiseAssert(interrupt);
		m_interrupt = true;
	}
	else if (!m_negateIrq)
	{
		// Pulse is over
		m_interrupt = false;
	}
	predictNextSync(timeKeeper);
}

void Timer::predictNextSync(TimeKeeper& timeKeeper)
{
	if (!m_targetIrq)
	{
		// No IRQ enabled, we don't need to be called back.
		timeKeeper.noSyncNeeded(m_instance);
		return;
	}

	uint16_t countDown = 0xffff - m_counter + m_target;
	if (m_counter <= m_target)
		countDown = m_target - m_counter;

	// Convert from timer count to CPU cycles
	Cycles delta = m_period.getFp() * ((Cycles)countDown + 1);
	delta -= m_phase.getFp();

	// Round up to the next CPU cycle
	delta = FracCycles::fromFp(delta).ceil();
	timeKeeper.setNextSyncDelta(m_instance, delta);
}

bool Timer::needsGpu()
{
	assert(!m_useSync, "Sync mode not supported!");
	return ::needsGpu(m_clockSource.clock(m_instance));
}

uint16_t Timer::getMode()
{
	uint16_t mode = 0;

	mode |= (uint16_t)m_useSync;
	mode |= ((uint16_t)m_sync) << 1;
	mode |= ((uint16_t)m_targetWrap) << 3;
	mode |= ((uint16_t)m_targetIrq) << 4;
	mode |= ((uint16_t)m_wrapIrq) << 5;
	mode |= ((uint16_t)m_repeatIrq) << 6;
	mode |= ((uint16_t)m_negateIrq) << 7;
	mode |= ((uint16_t)m_clockSource.getClockSource()) << 8;
	mode |= ((uint16_t)(!m_interrupt)) << 10;
	mode |= ((uint16_t)m_targetReached) << 11;
	mode |= ((uint16_t)m_overflowReached) << 12;

	// Reading mode resets those flags
	m_targetReached = false;
	m_overflowReached = false;

	return mode;
}

void Timer::setMode(uint16_t value)
{
	m_useSync = value & 1;
	m_sync = (SyncTimer)((value >> 1) & 3);
	m_targetWrap = (value >> 3) & 1;
	m_targetIrq = (value >> 4) & 1;
	m_wrapIrq = (value >> 5) & 1;
	m_repeatIrq = (value >> 6) & 1;
	m_negateIrq = (value >> 7) & 1;
	m_clockSource = ClockSource::fromField((value >> 8) & 3);
	
	// Writing to mode resets the interrupt flag
	m_interrupt = false;

	// Writing to mode resets the counter
	m_counter = 0;

	assert(!m_wrapIrq, "Wrap IRQ not supported");
	assert(!((m_wrapIrq || m_targetIrq) && !m_repeatIrq), "One shot timer interrupts are not supported");
	assert(!m_negateIrq, "Only pulse interrupts are supported");
	assert(!m_useSync, "Sync mode is not supported");
}

uint16_t Timer::getTarget() const
{
	return m_target;
}

void Timer::setTarget(uint16_t value)
{
	m_target = value;
}

uint16_t Timer::getCounter() const
{
	return m_counter;
}

void Timer::setCounter(uint16_t value)
{
	m_counter = value;
}

// ************* Timers implementation ******************
template<typename T>
void Timers::store(TimeKeeper& timeKeeper, InterruptState& irqState,
	Gpu& gpu, uint32_t offset, T value)
{
	assert((std::is_same<uint32_t, T>::value || std::is_same<uint16_t, T>::value), "Unhandled timer store");

	uint32_t instance = offset >> 4;

	Timer& timer = m_timers[instance];
	timer.sync(timeKeeper, irqState);

	switch (offset & 0xf)
	{
	case 0:
		timer.setCounter(value);
		break;
	case 4:
		timer.setMode(value);
		break;
	case 8:
		timer.setTarget(value);
		break;
	default:
		LOG("Unhandled timer register");
		return;
	}

	if (timer.needsGpu())
		gpu.sync(timeKeeper, irqState);

	timer.reconfigure(gpu, timeKeeper);
}

template void Timers::store<uint32_t>(TimeKeeper&, InterruptState&, Gpu&, uint32_t, uint32_t);
template void Timers::store<uint16_t>(TimeKeeper&, InterruptState&, Gpu&, uint32_t, uint16_t);
template void Timers::store<uint8_t >(TimeKeeper&, InterruptState&, Gpu&, uint32_t, uint8_t);

template<typename T>
T Timers::load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset)
{
	assert((std::is_same<uint32_t, T>::value || std::is_same<uint16_t, T>::value), "Unhandled timer load");

	uint32_t instance = offset >> 4;

	Timer& timer = m_timers[instance];
	timer.sync(timeKeeper, irqState);

	T value = 0x0;
	switch (offset & 0xf)
	{
	case 0:
		value = timer.getCounter();
		break;
	case 4:
		value = timer.getMode();
		break;
	case 8:
		value = timer.getTarget();
		break;
	default:
		LOG("Unhandled timer register");
		return ~0;
	}

	return (uint32_t)value;
}

template uint32_t Timers::load<uint32_t>(TimeKeeper&, InterruptState&, uint32_t);
template uint16_t Timers::load<uint16_t>(TimeKeeper&, InterruptState&, uint32_t);
template uint8_t  Timers::load<uint8_t >(TimeKeeper&, InterruptState&, uint32_t);

void Timers::videoTimingsChanged(TimeKeeper& timeKeeper, InterruptState& irqState, Gpu& gpu)
{
	size_t timersCount = _countof(m_timers);
	for (size_t timerIdx = 0; timerIdx < timersCount; ++timerIdx)
	{
		Timer& timer = m_timers[timerIdx];
		if (timer.needsGpu())
		{
			timer.sync(timeKeeper, irqState);
			timer.reconfigure(gpu, timeKeeper);
		}
	}
}

void Timers::sync(TimeKeeper& timeKeeper, InterruptState& irqState)
{
	if (timeKeeper.needsSync(Peripheral::PERIPHERAL_TIMER0))
		m_timers[0].sync(timeKeeper, irqState);
	if (timeKeeper.needsSync(Peripheral::PERIPHERAL_TIMER1))
		m_timers[1].sync(timeKeeper, irqState);
	if (timeKeeper.needsSync(Peripheral::PERIPHERAL_TIMER2))
		m_timers[2].sync(timeKeeper, irqState);
}
