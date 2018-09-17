#pragma once

#include "pscx_bios.h"

struct Instruction
{
	Instruction(uint32_t opcode) : m_instruction(opcode) {}

	// Return bits [31:26] of the instruction
	uint32_t getInstructionCode() const;
	
	// Return register source index in bits [25:21]
	uint32_t getRegisterSourceIndex() const;

	// Return register target index in bits [20:16]
	uint32_t getRegisterTargetIndex() const;

	// Return register destination index in bits [15:11]
	uint32_t getRegisterDestinationIndex() const;

	// Return immediate value in bits [15:0]
	uint32_t getImmediateValue() const;

	// Return immediate value in bits [16:0] as a sign-extended 32 bit value
	uint32_t getSignExtendedImmediateValue() const;

	uint32_t m_instruction;
};
