#pragma once

#include "pscx_bios.h"
#include "pscx_memory.h"

using namespace pscx_memory;

struct Instruction
{
	enum class InstructionStatus
	{
		INSTRUCTION_STATUS_LOADED_SUCCESSFULLY,
		INSTRUCTION_STATUS_NOT_IMPLEMENTED,
		INSTRUCTION_STATUS_UNALIGNED_ACCESS,
		INSTRUCTION_STATUS_UNHANDLED_FETCH,
		INSTRUCTION_STATUS_COUNT
	};

	Instruction() : m_instruction(0x0), m_instructionStatus(InstructionStatus::INSTRUCTION_STATUS_LOADED_SUCCESSFULLY) {};
	Instruction(uint32_t opcode, InstructionStatus instructionStatus = InstructionStatus::INSTRUCTION_STATUS_LOADED_SUCCESSFULLY) : m_instruction(opcode), m_instructionStatus(instructionStatus) {}

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

// Instruction cache line
struct ICacheLine
{
	// The cache starts in a random state. In order to catch
	// missbehaving software we fill them with "trap" values.
	ICacheLine() :
		m_tagValid(0x0) // Tag is 0, all line is valid.
	{
		for (size_t i = 0; i < 4; ++i)
			m_cacheLine[i] = 0x00bad0d; // BREAK opcode
	}

	// Return the cacheline's tag
	uint32_t getTag() const;

	// Return the cacheline's first valid word
	uint32_t getValidIndex() const;

	// Set the cacheline's tag and valid bits. 'pc' is the first
	// valid PC in the cacheline.
	void setTagValid(uint32_t pc);

	// Invalidate the entire cacheline by pushing the index out of
	// range. Doesn't change the tag or contents of the line.
	void invalidate();

	Instruction getInstruction(uint32_t index) const;
	void setInstruction(uint32_t index, const Instruction& instruction);

private:
	// Tag: high 22 bits of the address associated with this cacheline
	// Valid bits: 3 bit index of the first valid word in line.
	uint32_t m_tagValid;

	// Four words per line
	Instruction m_cacheLine[4];
};
