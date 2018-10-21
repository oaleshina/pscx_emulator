#pragma once

#include "pscx_common.h"
#include "pscx_interconnect.h"
#include "pscx_instruction.h"
#include "pscx_memory.h"

using namespace pscx_memory;

// CPU state
struct Cpu
{
	// 0xbfc00000 PC reset value at the beginning of the BIOS
	Cpu(Interconnect inter) :
		m_pc(0xbfc00000),
		m_nextInstruction(0x0), // NOP
		m_inter(inter),
		m_sr(0x0),
		m_load(RegisterIndex(0x0), 0x0)
	{
		// Reset registers values to 0xdeadbeef
		memset(m_regs, 0xdeadbeef, sizeof(m_regs));
		memcpy(m_outRegs, m_regs, sizeof(m_regs));

		// $zero is hardwired to 0
		m_regs[0] = 0x0;
		m_outRegs[0] = 0x0;
	}

	enum InstructionType
	{
		INSTRUCTION_TYPE_LUI,
		INSTRUCTION_TYPE_ORI,
		INSTRUCTION_TYPE_SW,
		INSTRUCTION_TYPE_SLL,
		INSTRUCTION_TYPE_ADDIU,
		INSTRUCTION_TYPE_J,
		INSTRUCTION_TYPE_OR,
		INSTRUCTION_TYPE_COP0,
		INSTRUCTION_TYPE_MTC0,
		INSTRUCTION_TYPE_BNE,
		INSTRUCTION_TYPE_ADDI,
		INSTRUCTION_TYPE_LW,
		INSTRUCTION_TYPE_SLTU,
		INSTRUCTION_TYPE_ADDU,
		INSTRUCTION_TYPE_SH,
		INSTRUCTION_TYPE_JAL,
		INSTRUCTION_TYPE_ANDI,
		INSTRUCTION_TYPE_SB,
		INSTRUCTION_TYPE_JR,
		INSTRUCTION_TYPE_LB,
		INSTRUCTION_TYPE_BEQ,
		INSTRUCTION_TYPE_CACHE_ISOLATED,
		INSTRUCTION_TYPE_NOT_IMPLEMENTED, // temporal enum variable for debugging
		INSTRUCTION_TYPE_UNKNOWN,
		INSTRUCTION_TYPE_COUNT
	};

	InstructionType runNextInstuction();

	const uint32_t* getRegistersPtr() const;
	const std::vector<uint32_t>& getInstructionsDump() const;

private:
	struct RegisterData
	{
		RegisterData(RegisterIndex registerIndex, uint32_t registerValue) :
			m_registerIndex(registerIndex),
			m_registerValue(registerValue)
		{}

		RegisterIndex m_registerIndex;
		uint32_t      m_registerValue;
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

	// Second set of registers used to emulate the load delay slot accurately
	// They contain the output of the current instruction
	uint32_t m_outRegs[32];

	// Cop0 register 12: Status Register
	uint32_t m_sr;

	// Load initiated by the current instruction
	RegisterData m_load;

	// Next instruction to be executed, used to simulate 
	// the branch delay slot
	Instruction m_nextInstruction;

	// Memory interface
	Interconnect m_inter;

	// Array for storing instructions for logging
	std::vector<uint32_t> m_debugInstructions;

	// Load 32 bit value from the interconnect
	Instruction load32(uint32_t addr) const;

	// Load 8 bit value from the memory
	Instruction load8(uint32_t addr) const;

	void store32(uint32_t addr, uint32_t value); // Store 32 bit value into the memory
	void store16(uint32_t addr, uint16_t value); // Store 16 bit value into the memory
	void store8 (uint32_t addr, uint8_t  value); // Store 8 bit value into the memory

	InstructionType decodeAndExecute(const Instruction& instruction);

	// Opcodes
	InstructionType opcodeLUI  (const Instruction& instruction); // Load Upper Immediate
	InstructionType opcodeORI  (const Instruction& instruction); // Bitwise Or Immediate
	InstructionType opcodeSW   (const Instruction& instruction); // Store Word
	InstructionType opcodeSLL  (const Instruction& instruction); // Shift left logical
	InstructionType opcodeADDIU(const Instruction& instruction); // Add Immediate Unsigned
	InstructionType opcodeJ    (const Instruction& instruction); // Jump
	InstructionType opcodeOR   (const Instruction& instruction); // Bitwise Or
	InstructionType opcodeCOP0 (const Instruction& instruction); // Instruction for coprocessor 0
	InstructionType opcodeMTC0 (const Instruction& instruction); // Move to coprocessor 0
	InstructionType opcodeBNE  (const Instruction& instruction); // Branch if not equal
	InstructionType opcodeADDI (const Instruction& instruction); // Add Immediate ( generates exception if addition overflows )
	InstructionType opcodeLW   (const Instruction& instruction); // Load word
	InstructionType opcodeSLTU (const Instruction& instruction); // Set on less than unsigned
	InstructionType opcodeADDU (const Instruction& instruction); // Add unsigned
	InstructionType opcodeSH   (const Instruction& instruction); // Store halfword
	InstructionType opcodeJAL  (const Instruction& instruction); // Jump and link
	InstructionType opcodeANDI (const Instruction& instruction); // Bitwise and immediate
	InstructionType opcodeSB   (const Instruction& instruction); // Store byte
	InstructionType opcodeJR   (const Instruction& instruction); // Jump register
	InstructionType opcodeLB   (const Instruction& instruction); // Load byte ( signed )
	InstructionType opcodeBEQ  (const Instruction& instruction); // Branch if equal

	uint32_t getRegisterValue(RegisterIndex registerIndex) const;
	void setRegisterValue(RegisterIndex registerIndex, uint32_t value);
};
