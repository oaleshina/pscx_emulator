#pragma once

#include <cstdint>
#include "pscx_interrupts.h"

// Exception types ( sd stored in the 'CAUSE' register )
enum Exception
{
	// Interrupt Request
	EXCEPTION_INTERRUPT = 0x0,

	// Address error on load
	EXCEPTION_LOAD_ADDRESS_ERROR = 0x4,

	// Address error on store
	EXCEPTION_STORE_ADDRESS_ERROR = 0x5,

	// System call ( caused by the SYSCALL opcode )
	EXCEPTION_SYSCALL = 0x8,

	// Breakpoint ( caused by the BREAK opcode )
	EXCEPTION_BREAK = 0x9,

	// CPU encountered an unknown instruction
	EXCEPTION_UNKNOWN_INSTRUCTION = 0xa,

	// Unsupported coprocessor operation
	EXCEPTION_COPROCESSOR_ERROR = 0xb,

	// Arithmetic overflow
	EXCEPTION_OVERFLOW = 0xc
};

// Coprocessor 0: System control
struct Cop0
{
	Cop0() :
		m_sr(0x0),
		m_cause(0x0),
		m_epc(0x0)
	{}

	uint32_t getStatusRegister() const;
	void setStatusRegister(uint32_t sr);

	// Retreive the value of the CAUSE register. We need the
	// InterruptState because bit 10 is wired to the current external
	// interrupt (no latch, ack'ing the interrupt in the external
	// controller resets the value in this register).
	uint32_t getCauseRegister(InterruptState irqState) const;

	uint32_t getExceptionPCRegister() const;

	bool isCacheIsolated() const;

	// Update SR, CAUSE and EPC when an exception is
	// triggered. Returns the address of the exteption handler.
	uint32_t enterException(Exception cause, uint32_t pc, bool inDelaySlot);

	// The opposite of 'enter_exception': shift SR's mode the other way around,
	// discarding the current state. Doesn't touch CAUSE or EPC however.
	void returnFromException();

	// Return true if the interrupts are enabled on the CPU (SR
	// "Current Interrupt Enable" bit is set)
	bool irqEnabled() const;

	// Return true if an interrupt (either software or hardware) is pending
	bool isIrqActive(InterruptState irqState);

private:
	// Cop0 register 12: Status register
	uint32_t m_sr;

	// Cop0 register 13: Cause register
	uint32_t m_cause;

	// Cop0 register 14: Exception PC
	uint32_t m_epc;
};

