#include "pscx_cop0.h"
#include <iostream>

uint32_t Cop0::getStatusRegister() const
{
	return m_sr;
}

void Cop0::setStatusRegister(uint32_t sr)
{
	m_sr = sr;
}

uint32_t Cop0::getCauseRegister(InterruptState irqState) const
{
	return m_cause | (((uint32_t)irqState.isActiveInterrupt()) << 10);
}

uint32_t Cop0::getExceptionPCRegister() const
{
	return m_epc;
}

bool Cop0::isCacheIsolated() const
{
	return m_sr & 0x10000;
}

uint32_t Cop0::enterException(Exception cause, uint32_t pc, bool inDelaySlot)
{
	// Shift bits [5:0] of 'sr' two bits to the left. Those bits
	// are three pairs of Interrupt Enable/User Mode bits behaving
	// like a stack 3 entries deep. Entering an exception pushes a pair
	// of zeroes by left shifting the stack which disables
	// interrupts and puts the CPU in kernel mode. The original third entry
	// is discarded (it's up to the kernel to handle more than two recursive exception levels).
	uint32_t mode = m_sr & 0x3f;
	m_sr &= (~0x3f);
	m_sr |= (mode << 2) & 0x3f;

	// Update 'CAUSE' register with the exception code (bits [6:2])
	m_cause &= (~0x7c);
	m_cause |= ((uint32_t)cause) << 2;

	if (inDelaySlot)
	{
		// When an exception occurs in a delay slot 'EPC' points
		// to the branch instruction and bit of 'CAUSE' is set.
		m_epc = pc - 4;
		m_cause |= (1 << 31);
	}
	else
	{
		m_epc = pc;
		m_cause &= (~(1 << 31));
	}

	// The address of the exception handler address depends on the
	// value of the BEV bit in SR
	if (m_sr & (1 << 22))
		return 0xbfc00180;
	return 0x80000080;
}

void Cop0::returnFromException()
{
	//std::cout << "return from exception" << std::endl;
	uint32_t mode = m_sr & 0x3f;
	m_sr &= (~0xf);
	m_sr |= (mode >> 2);
}

bool Cop0::irqEnabled() const
{
	return m_sr & 0x1;
}

bool Cop0::isIrqActive(InterruptState irqState)
{
	uint32_t cause = getCauseRegister(irqState);

	// Bits [8:9] of CAUSE contain two software interrupts
	// while bit 10 is wired to the external interrupt's state.
	// They can be individually masked using SR's bits [8:10]
	uint32_t pending = (cause & m_sr) & 0x700;

	// Finally bit 0 of SR can be used to mask interrupts globally
	// on the CPU so we must check that
	return irqEnabled() && pending;
}
