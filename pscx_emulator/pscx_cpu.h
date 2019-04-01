#pragma once

#include "pscx_common.h"
#include "pscx_interconnect.h"
#include "pscx_instruction.h"
#include "pscx_memory.h"
#include "pscx_timekeeper.h"
#include "pscx_cop0.h"
#include "pscx_gte.h"
#include "pscx_gamepad.h"

using namespace pscx_memory;

// Playstation CPU clock in MHz
const uint32_t CPU_FREQ_HZ = 33'868'500;

// CPU state
struct Cpu
{
	// 0xbfc00000 PC reset value at the beginning of the BIOS
	Cpu(Interconnect inter) :
		m_pc(0xbfc00000),
		m_nextPc(0xbfc00004),
		m_currentPc(0x0),
		m_nextInstruction(0x0), // NOP
		m_inter(inter),
		m_load(RegisterIndex(0x0), 0x0),
		m_branch(false),
		m_delaySlot(false)
	{
		// Reset registers values to 0xdeadbeef
		memset(m_regs, 0xdeadbeef, sizeof(m_regs));
		memcpy(m_outRegs, m_regs, sizeof(m_regs));

		m_hi = 0xdeadbeef;
		m_lo = 0xdeadbeef;

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
		INSTRUCTION_TYPE_AND,
		INSTRUCTION_TYPE_COP0,
		INSTRUCTION_TYPE_MTC0,
		INSTRUCTION_TYPE_MFC0,
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
		INSTRUCTION_TYPE_ADD,
		INSTRUCTION_TYPE_BGTZ,
		INSTRUCTION_TYPE_BLEZ,
		INSTRUCTION_TYPE_LBU,
		INSTRUCTION_TYPE_JALR,
		INSTRUCTION_TYPE_BXX,
		INSTRUCTION_TYPE_SLTI,
		INSTRUCTION_TYPE_SUBU,
		INSTRUCTION_TYPE_SRA,
		INSTRUCTION_TYPE_DIV,
		INSTRUCTION_TYPE_MFLO,
		INSTRUCTION_TYPE_SRL,
		INSTRUCTION_TYPE_SLTIU,
		INSTRUCTION_TYPE_DIVU,
		INSTRUCTION_TYPE_MFHI,
		INSTRUCTION_TYPE_SLT,
		INSTRUCTION_TYPE_SYSCALL,
		INSTRUCTION_TYPE_MTLO,
		INSTRUCTION_TYPE_MTHI,
		INSTRUCTION_TYPE_RFE,
		INSTRUCTION_TYPE_LHU,
		INSTRUCTION_TYPE_SLLV,
		INSTRUCTION_TYPE_LH,
		INSTRUCTION_TYPE_NOR,
		INSTRUCTION_TYPE_SRAV,
		INSTRUCTION_TYPE_SRLV,
		INSTRUCTION_TYPE_MULTU,
		INSTRUCTION_TYPE_XOR,
		INSTRUCTION_TYPE_BREAK,
		INSTRUCTION_TYPE_MULT,
		INSTRUCTION_TYPE_SUB,
		INSTRUCTION_TYPE_XORI,
		INSTRUCTION_TYPE_COP1,
		INSTRUCTION_TYPE_COP2,
		INSTRUCTION_TYPE_CTC2,
		INSTRUCTION_TYPE_COP3,
		INSTRUCTION_TYPE_LWL,
		INSTRUCTION_TYPE_LWR,
		INSTRUCTION_TYPE_SWL,
		INSTRUCTION_TYPE_SWR,
		INSTRUCTION_TYPE_LWC0,
		INSTRUCTION_TYPE_LWC1,
		INSTRUCTION_TYPE_LWC2,
		INSTRUCTION_TYPE_LWC3,
		INSTRUCTION_TYPE_SWC0,
		INSTRUCTION_TYPE_SWC1,
		INSTRUCTION_TYPE_SWC2,
		INSTRUCTION_TYPE_SWC3,
		//INSTRUCTION_TYPE_ILLEGAL,
		INSTRUCTION_TYPE_CACHE_ISOLATED,
		INSTRUCTION_TYPE_NOT_IMPLEMENTED, // temporal enum variable for debugging
		INSTRUCTION_TYPE_UNKNOWN,
		INSTRUCTION_TYPE_UNALIGNED,
		INSTRUCTION_TYPE_OVERFLOW,
		INSTRUCTION_TYPE_EXCEPTION_INTERRUPT,
		INSTRUCTION_TYPE_COUNT
	};

	InstructionType runNextInstuction();

	const uint32_t* getRegistersPtr() const;
	const std::vector<uint32_t>& getInstructionsDump() const;

	std::vector<Profile*> getPadProfiles();

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

	// Struct used to keep track of each peripheral's emulation
	// advancement and synchronize them when needed
	TimeKeeper m_timeKeeper;

	// Special purpose registers
	uint32_t m_pc; // program counter register: points to the next instruction
	uint32_t m_nextPc; // next value for PC, used to simulate the branch delay slot
	uint32_t m_currentPc; // address of the instruction currently being executed. Used for setting the EPC in exceptions

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

	// Instruction cache (256 4-word cachelines)
	ICacheLine m_icache[0x100];

	// Coprocessor 0: System control
	Cop0 m_cop0;

	// Coprocessor 2: Geometry Transform Engine
	Gte m_gte;

	// HI register for division remainder and multiplication high result
	uint32_t m_hi;

	// LO register for division quotient and multiplication low result
	uint32_t m_lo;

	// Load initiated by the current instruction
	RegisterData m_load;

	// Next instruction to be executed, used to simulate 
	// the branch delay slot
	Instruction m_nextInstruction;

	// Memory interface
	Interconnect m_inter;

	// Set by the current instruction if a branch occured and the
	// next instruction will be in the delay slot
	bool m_branch;

	// Set if the current instruction executes in the delay slot
	bool m_delaySlot;

	// Array for storing instructions for logging
	std::vector<uint32_t> m_debugInstructions;

	template<typename T>
	Instruction load(uint32_t addr);

	template<typename T>
	void store(uint32_t addr, T value);

	// Handle writes when the cache is isolated
	template<typename T> 
	void cacheMaintenance(uint32_t addr, T value);

	// Fetch the instruction at 'currentPC' through the instruction cache
	Instruction fetchInstruction();
	InstructionType decodeAndExecute(const Instruction& instruction);

	// Opcodes
	InstructionType opcodeLUI  (const Instruction& instruction); // Load Upper Immediate
	InstructionType opcodeORI  (const Instruction& instruction); // Bitwise Or Immediate
	InstructionType opcodeSW   (const Instruction& instruction); // Store Word
	InstructionType opcodeSLL  (const Instruction& instruction); // Shift left logical
	InstructionType opcodeADDIU(const Instruction& instruction); // Add Immediate Unsigned
	InstructionType opcodeJ    (const Instruction& instruction); // Jump
	InstructionType opcodeOR   (const Instruction& instruction); // Bitwise Or
	InstructionType opcodeAND  (const Instruction& instruction); // Bitwise And
	InstructionType opcodeCOP0 (const Instruction& instruction); // Instruction for coprocessor 0
	InstructionType opcodeMTC0 (const Instruction& instruction); // Move to coprocessor 0
	InstructionType opcodeMFC0 (const Instruction& instruction); // Move from coprocessor 0
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
	InstructionType opcodeADD  (const Instruction& instruction); // Add ( generates an exception on signed overflow )
	InstructionType opcodeBGTZ (const Instruction& instruction); // Branch if greater than zero
	InstructionType opcodeBLEZ (const Instruction& instruction); // Branch if less than or equal to zero
	InstructionType opcodeLBU  (const Instruction& instruction); // Load byte unsigned
	InstructionType opcodeJALR (const Instruction& instruction); // Jump and link register
	InstructionType opcodeBXX  (const Instruction& instruction); // Various branch instructions: BGEZ, BLTZ, BGEZAL, BLTZAL. Bits [20:16] are used to figure out which one to use
	InstructionType opcodeSLTI (const Instruction& instruction); // Set if less than immediate
	InstructionType opcodeSUBU (const Instruction& instruction); // Substract unsigned
	InstructionType opcodeSRA  (const Instruction& instruction); // Shift right arithmetic
	InstructionType opcodeDIV  (const Instruction& instruction); // Division ( signed )
	InstructionType opcodeMFLO (const Instruction& instruction); // Move from LO
	InstructionType opcodeSRL  (const Instruction& instruction); // Shift right logical
	InstructionType opcodeSLTIU(const Instruction& instruction); // Set if less than immediate unsigned
	InstructionType opcodeDIVU (const Instruction& instruction); // Division unsigned
	InstructionType opcodeSLT  (const Instruction& instruction); // Set on less than ( signed )
	InstructionType opcodeMFHI (const Instruction& instruction); // Move from HI
	InstructionType opcodeSYSCALL(const Instruction& instruction); // System call
	InstructionType opcodeMTLO (const Instruction& instruction); // Move to LO
	InstructionType opcodeMTHI (const Instruction& instruction); // Move to HI
	InstructionType opcodeRFE  (const Instruction& instruction); // Return from exception
	InstructionType opcodeLHU  (const Instruction& instruction); // Load halfword unsigned
	InstructionType opcodeSLLV (const Instruction& instruction); // Shift left logical variable
	InstructionType opcodeLH   (const Instruction& instruction); // Load halfword
	InstructionType opcodeNOR  (const Instruction& instruction); // Bitwise not or
	InstructionType opcodeSRAV (const Instruction& instruction); // Shift right arithmetic variable
	InstructionType opcodeSRLV (const Instruction& instruction); // Shift right logical variable
	InstructionType opcodeMULTU(const Instruction& instruction); // Multiply unsigned
	InstructionType opcodeXOR  (const Instruction& instruction); // Bitwise exclusive or
	InstructionType opcodeBREAK(const Instruction& instruction); // Break
	InstructionType opcodeMULT (const Instruction& instruction); // Multiply ( signed )
	InstructionType opcodeSUB  (const Instruction& instruction); // Substract and check for signed overflow
	InstructionType opcodeXORI (const Instruction& instruction); // Bitwise exclusive or immediate
	InstructionType opcodeCOP1 (const Instruction& instruction); // Coprocessor 1 opcode ( does not exist on the Playstation )
	InstructionType opcodeCOP2 (const Instruction& instruction); // Coprocessor 2 opcode ( Geometry Transform Engine )
	InstructionType opcodeCTC2 (const Instruction& instruction); // Move to coprocessor 2 Control register
	InstructionType opcodeCOP3 (const Instruction& instruction); // Coprocessor 3 opcode ( does not exist on the Playstation )
	InstructionType opcodeLWL  (const Instruction& instruction); // Load word left ( little-endian only implementation )
	InstructionType opcodeLWR  (const Instruction& instruction); // Load word right ( little-endian only implementation )
	InstructionType opcodeSWL  (const Instruction& instruction); // Store word left ( little-endian only implementation )
	InstructionType opcodeSWR  (const Instruction& instruction); // Store word right ( little-endian only implementation )
	InstructionType opcodeLWC0 (const Instruction& instruction); // Load word in Coprocessor 0
	InstructionType opcodeLWC1 (const Instruction& instruction); // Load word in Coprocessor 1
	InstructionType opcodeLWC2 (const Instruction& instruction); // Load word in Coprocessor 2
	InstructionType opcodeLWC3 (const Instruction& instruction); // Load word in Coprocessor 3
	InstructionType opcodeSWC0 (const Instruction& instruction); // Store word in Coprocessor 0
	InstructionType opcodeSWC1 (const Instruction& instruction); // Store word in Coprocessor 1
	InstructionType opcodeSWC2 (const Instruction& instruction); // Store word in Coprocessor 2
	InstructionType opcodeSWC3 (const Instruction& instruction); // Store word in Coprocessor 3
	InstructionType opcodeIllegal(const Instruction& instruction); // Illegal instruction

	void branch(uint32_t offset);
	void exception(Exception cause); // Trigger an exception

	uint32_t getRegisterValue(RegisterIndex registerIndex) const;
	void setRegisterValue(RegisterIndex registerIndex, uint32_t value);
};
