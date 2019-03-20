#include "pscx_instruction.h"

uint32_t Instruction::getInstructionCode() const
{
	return m_instruction >> 26;
}

RegisterIndex Instruction::getRegisterSourceIndex() const
{
	return RegisterIndex((m_instruction >> 21) & 0x1f);
}

RegisterIndex Instruction::getRegisterTargetIndex() const
{
	return RegisterIndex((m_instruction >> 16) & 0x1f);
}

RegisterIndex Instruction::getRegisterDestinationIndex() const
{
	return RegisterIndex((m_instruction >> 11) & 0x1f);
}

uint32_t Instruction::getImmediateValue() const
{
	return m_instruction & 0xffff;
}

uint32_t Instruction::getSignExtendedImmediateValue() const
{
	return (int16_t)(m_instruction & 0xffff);
}

uint32_t Instruction::getSubfunctionInstructionCode() const
{
	return m_instruction & 0x3f;
}

uint32_t Instruction::getShiftImmediateValue() const
{
	return (m_instruction >> 6) & 0x1f;
}

uint32_t Instruction::getJumpTargetValue() const
{
	return m_instruction & 0x3ffffff;
}

uint32_t Instruction::getCopOpcodeValue() const
{
	return (m_instruction >> 21) & 0x1f;
}

uint32_t Instruction::getInstructionOpcode() const
{
	return m_instruction;
}

Instruction::InstructionStatus Instruction::getInstructionStatus() const
{
	return m_instructionStatus;
}

// ***************** ICacheLine implementation ******************
uint32_t ICacheLine::getTag() const
{
	return m_tagValid & 0xfffff000;
}

uint32_t ICacheLine::getValidIndex() const
{
	// We store the valid bits in bits [4:2], this way we can just
	// mask the PC value  in 'setTagValid' without having to shuffle
	// the bits around
	return (m_tagValid >> 2) & 0x7;
}

void ICacheLine::setTagValid(uint32_t pc)
{
	m_tagValid = pc & 0x7ffff00c;
}

void ICacheLine::invalidate()
{
	// Setting bit 4 means that the value returned by getValidIndex
	// will be in the range [4, 7] which is outside the valid cacheline
	// index range [0, 3]
	m_tagValid |= 0x10;
}

Instruction ICacheLine::getInstruction(uint32_t index) const
{
	return m_cacheLine[index];
}

void ICacheLine::setInstruction(uint32_t index, const Instruction& instruction)
{
	m_cacheLine[index] = instruction;
}
