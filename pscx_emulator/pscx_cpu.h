#pragma once

#include "pscx_interconnect.h"
#include "pscx_instruction.h"

// CPU state
struct Cpu
{
	// 0xbfc00000 PC reset value at the beginning of the BIOS
	Cpu(Interconnect inter) :
		m_pc(0xbfc00000),
		m_inter(inter)
	{
		// Reset registers values to 0xdeadbeef
		memset(m_regs, 0xdeadbeef, sizeof(uint32_t) * 32);

		// $zero is hardwired to 0
		m_regs[0] = 0x0;
	}

	enum InstructionType
	{
		INSTRUCTION_TYPE_LUI,
		INSTRUCTION_TYPE_ORI,
		INSTRUCTION_TYPE_SW,
		INSTRUCTION_TYPE_UNKNOWN
	};

	// Special purpose registers
	uint32_t m_pc; // program counter register

	// General purpose registers
	// m_regs[0]                     $zero      Always zero
	// m_regs[1]                     $at        Assembler temporary
	// m_regs[2],      m_regs[3]     $v0,v1     Function return values
	// m_regs[4]  ...  m_regs[7]     $a0...$a3  Function arguments
	// m_regs[8]  ...  m_regs[15]    $t0...$t7  Temporary registers
	// m_regs[16] ...  m_regs[23]    $s0...$s7  Saved registers
	// m_regs[24],     m_regs[25]    $t8,$t9    Temporary registers
	// m_regs[26],     m_regs[27]    $k0,$k1    Kernel reserved registers
	// m_regs[28]                    $gp        Global pointer
	// m_regs[29]                    $sp        Stack pointer
	// m_regs[30]                    $fp        Frame pointer
	// m_regs[31]                    $ra        Function return address
	uint32_t m_regs[32];

	// Memory interface
	Interconnect m_inter;

	// Load 32 bit value from the interconnect
	uint32_t load32(uint32_t addr);

	// Store 32 bit value into the memory
	void store32(uint32_t addr, uint32_t value);

	InstructionType decodeAndExecute(Instruction instruction);
	InstructionType runNextInstuction();

	// Load Upper Immediate
	InstructionType opcodeLUI(Instruction instruction);

	// Bitwise Or Immediate
	InstructionType opcodeORI(Instruction instruction);

	// Store Word
	InstructionType opcodeSW(Instruction instruction);

	uint32_t getRegisterValue(uint32_t index) const;
	void setRegisterValue(uint32_t index, uint32_t value);
};
