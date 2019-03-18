#include <cassert>

#include "pscx_interrupts.h"
#include "pscx_common.h"

bool InterruptState::isActiveInterrupt() const
{
	return m_status & m_mask;
}

uint16_t InterruptState::getInterruptStatus() const
{
	return m_status;
}

void InterruptState::acknowledgeInterrupts(uint16_t ack)
{
	m_status &= ack;
}

uint16_t InterruptState::getInterruptMask() const
{
	return m_mask;
}

void InterruptState::setInterruptMask(uint16_t mask)
{
	Interrupt supported[] = { Interrupt::INTERRUPT_VBLANK, 
							  Interrupt::INTERRUPT_CDROM, 
							  Interrupt::INTERRUPT_DMA,
							  Interrupt::INTERRUPT_TIMER0,
							  Interrupt::INTERRUPT_TIMER1,
							  Interrupt::INTERRUPT_TIMER2,
							  Interrupt::INTERRUPT_PAD_MEMCARD };

	uint16_t rem = mask;
	for (size_t i = 0; i < _countof(supported); ++i)
		rem &= (~(1 << supported[i]));

	// Fails on assert. Used for debugging.
	assert(rem == 0x0, "Unsupported interrupt");
	m_mask = mask;
}

void InterruptState::raiseAssert(Interrupt which)
{
	m_status |= (1 << which);
}
