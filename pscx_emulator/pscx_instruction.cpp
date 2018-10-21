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
