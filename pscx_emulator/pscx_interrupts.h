#pragma once

#include <cstdint>

// The Playstation supports 10 interrupts
enum Interrupt
{
	// Display the vertical blanking
	INTERRUPT_VBLANK = 0
};

struct InterruptState
{
	InterruptState() :
		m_status(0x0),
		m_mask(0x0)
	{}

	// Return true if at least one interrupt is active and not masked
	bool isActiveInterrupt() const;

	uint16_t getInterruptStatus() const;

	// Acknowledge interrupts by writing 0 to the corresponding bit
	void acknowledgeInterrupts(uint16_t ack);

	uint16_t getInterruptMask() const;
	void setInterruptMask(uint16_t mask);

	void setHighStatusBits(Interrupt which);

private:
	// Interrupt status
	uint16_t m_status;

	// Interrupt mask
	uint16_t m_mask;
};

