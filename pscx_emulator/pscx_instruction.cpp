#include "pscx_instruction.h"

uint32_t Instruction::getInstructionCode() const
{
	return m_instruction >> 26;
}

uint32_t Instruction::getRegisterSourceIndex() const
{
	return (m_instruction >> 21) & 0x1f;
}

uint32_t Instruction::getRegisterTargetIndex() const
{
	return (m_instruction >> 16) & 0x1f;
}

uint32_t Instruction::getRegisterDestinationIndex() const
{
	return (m_instruction >> 11) & 0x1f;
}

uint32_t Instruction::getImmediateValue() const
{
	return m_instruction & 0xffff;
}

uint32_t Instruction::getSignExtendedImmediateValue() const
{
	return (int16_t)(m_instruction & 0xffff);
}