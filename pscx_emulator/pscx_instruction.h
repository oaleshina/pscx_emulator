#pragma once

#include "pscx_bios.h"
#include "pscx_memory.h"

using namespace pscx_memory;

struct Instruction
{
	enum InstructionStatus
	{
		INSTRUCTION_STATUS_LOADED_SUCCESSFULLY,
		INSTRUCTION_STATUS_NOT_IMPLEMENTED,
		INSTRUCTION_STATUS_UNALIGNED_ACCESS,
		INSTRUCTION_STATUS_UNHANDLED_FETCH,
		INSTRUCTION_STATUS_COUNT
	};

	Instruction(uint32_t opcode, InstructionStatus instructionStatus = INSTRUCTION_STATUS_LOADED_SUCCESSFULLY) : m_instruction(opcode), m_instructionStatus(instructionStatus) {}

	// Return bits [31:26] of the instruction
	uint32_t getInstructionCode() const;
	
	// Return register source index in bits [25:21]
	RegisterIndex getRegisterSourceIndex() const;

	// Return register target index in bits [20:16]
	RegisterIndex getRegisterTargetIndex() const;

	// Return register destination index in bits [15:11]
	RegisterIndex getRegisterDestinationIndex() const;

	// Return immediate value in bits [15:0]
	uint32_t getImmediateValue() const;

	// Return immediate value in bits [16:0] as a sign-extended 32 bit value
	uint32_t getSignExtendedImmediateValue() const;

	// Return bits [5:0] of the subfunction instruction code
	uint32_t getSubfunctionInstructionCode() const;

	// Return shift immediate value in bits [10:6]
	uint32_t getShiftImmediateValue() const;

	// Return jump target value in bits [25:0]
	uint32_t getJumpTargetValue() const;

	// Return coprocessor opcode value in bits [25:21]
	uint32_t getCopOpcodeValue() const;

	// Return instruction opcode
	uint32_t getInstructionOpcode() const;

	// Return instruction status
	InstructionStatus getInstructionStatus() const;

private:
	uint32_t m_instruction;
	InstructionStatus m_instructionStatus;
};
