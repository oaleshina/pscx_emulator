#include "pscx_interrupts.h"

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
	m_mask = mask;
}

void InterruptState::setHighStatusBits(Interrupt which)
{
	m_status |= (1 << which);
}
