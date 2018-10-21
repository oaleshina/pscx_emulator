#include "pscx_cpu.h"

#include <iostream>
#include <cassert>
#include <climits>

static void branch(uint32_t offset, uint32_t& pc);

const uint32_t* Cpu::getRegistersPtr() const
{
	return m_regs;
}

const std::vector<uint32_t>& Cpu::getInstructionsDump() const
{
	return m_debugInstructions;
}

Instruction Cpu::load32(uint32_t addr) const
{
	return m_inter.load32(addr);
}

Instruction Cpu::load8(uint32_t addr) const
{
	return m_inter.load8(addr);
}

void Cpu::store32(uint32_t addr, uint32_t value)
{
	m_inter.store32(addr, value);
}

void Cpu::store16(uint32_t addr, uint16_t value)
{
	m_inter.store16(addr, value);
}

void Cpu::store8(uint32_t addr, uint8_t value)
{
	m_inter.store8(addr, value);
}

// TODO: take a look how to get back error messages.
Cpu::InstructionType Cpu::decodeAndExecute(const Instruction& instruction)
{
	InstructionType instructionType = INSTRUCTION_TYPE_UNKNOWN;

	switch (instruction.getInstructionCode())
	{
	case 0b000000:
		// 0b000000 can introduce a number of instructions
		// To figure out which one we need, we should read bits [5:0]
		switch (instruction.getSubfunctionInstructionCode())
		{
		case /*SLL*/0b000000:
			instructionType = opcodeSLL(instruction);
			break;
		case /*OR*/0b100101:
			instructionType = opcodeOR(instruction);
			break;
		case/*SLTU*/0b101011:
			instructionType = opcodeSLTU(instruction);
			break;
		case/*ADDU*/0b100001:
			instructionType = opcodeADDU(instruction);
			break;
		case/*JR*/0b001000:
			instructionType = opcodeJR(instruction);
			break;
		default:
			LOG("Unhandled sub instruction 0x" << std::hex << instruction.getSubfunctionInstructionCode());
		}
		break;
	case /*LUI*/0b001111:
		//---------------------------------
		// TODO : call LUI instruction.
		// Rust:
		// 0b001111 => self.op_lui(instruction)
		//---------------------------------
		break;
	case /*ORI*/0b001101:
		instructionType = opcodeORI(instruction);
		break;
	case /*SW*/0b101011:
		instructionType = opcodeSW(instruction);
		break;
	case /*ADDIU*/0b001001:
		instructionType = opcodeADDIU(instruction);
		break;
	case/*J*/0b000010:
		instructionType = opcodeJ(instruction);
		break;
	case/*COP0*/0b010000:
		instructionType = opcodeCOP0(instruction);
		break;
	case/*BNE*/0b000101:
		instructionType = opcodeBNE(instruction);
		break;
	case /*ADDI*/0b001000:
		instructionType = opcodeADDI(instruction);
		break;
	case/*LW*/0b100011:
		instructionType = opcodeLW(instruction);
		break;
	case/*SH*/0b101001:
		instructionType = opcodeSH(instruction);
		break;
	case/*JAL*/0b000011:
		instructionType = opcodeJAL(instruction);
		break;
	case/*ANDI*/0b001100:
		instructionType = opcodeANDI(instruction);
		break;
	case/*SB*/0b101000:
		instructionType = opcodeSB(instruction);
		break;
	case/*LB*/0b100000:
		instructionType = opcodeLB(instruction);
		break;
	case/*BEQ*/0b000100:
		instructionType = opcodeBEQ(instruction);
		break;
	default:
		LOG("Unhandled instruction 0x" << std::hex << instruction.getInstructionCode());
	}

	m_debugInstructions.push_back(instruction.getInstructionOpcode());
	return instructionType;
}

//----------------------------------------------
// TODO : to implement the runNextInstruction function.
// It should fetch an instruction at program counter (PC) register,
// increment PC to point to the next instruction and
// execute instruction.
// 
// Rust:
// pub fn run_next instruction(&mut self) {
//     Use previously loaded instruction
//     let instruction = self.next_instruction;
//
//     Fetch instruction at PC
//     next_instruction = self.load32(pc);
//
//     if (next_instruction.status == UNALIGNED_ACCESS ||
//         next_instruction.status == UNHANDLED_FETCH)
//         ret INSTRUCTION_TYPE_UNKNOWN;
//
//     Increment PC to point to the next instruction.
//     self.pc += 4;
//
//     if (next_instruction.status == NOT_IMPLEMENTED)
//         ret INSTRUCTION_TYPE_NOT_IMPLEMENTED;
//
//     Execute the pending load ( if any, otherwise it will load $zero which is a NOP )
//     setRegisterValue works only on 'out_regs' so this operation won't be visible by the next instruction
//     let (reg, val) = self.load;
//
//     self.set_reg(reg, val);
//
//     We reset the load to target register 0 for the next instruction
//     self.load = RegisterData(RegisterIndex(0), 0); 
//
//     Execute any pending load
//     self.decode_and_execute(instruction);
//
//     Copy the output registers as input for the next instruction
//     self.regs = self.out_regs
// }
//----------------------------------------------
Cpu::InstructionType Cpu::runNextInstuction()
{
	// Fixme
	return INSTRUCTION_TYPE_UNKNOWN;
}

//--------------------------------------------------------------
// TODO : to implement the Load Upper Immediate function (LUI).
// It should load the immediate value into the high 16 bits of the target register.
//
// Rust:
// fn op_lui(&mut self, instruction: Instruction) {
//     let i = instruction.imm();  load immediate value
//     let t = instruction.t();    load target register
//
//     Low 16 bits are set to 0
//     let v = i << 16;
//
//     self.set_reg(t, v);
// }
//---------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeLUI(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_LUI;
}

Cpu::InstructionType Cpu::opcodeORI(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterTargetIndex(), instruction.getImmediateValue() | getRegisterValue(instruction.getRegisterSourceIndex()));
	return INSTRUCTION_TYPE_ORI;
}

Cpu::InstructionType Cpu::opcodeSW(const Instruction& instruction)
{
	if (m_sr & 0x10000)
	{
		// Cache is isolated, ignore write
		LOG("Ignoring store while cache is isolated");
		return INSTRUCTION_TYPE_CACHE_ISOLATED;
	}

	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();
	store32(getRegisterValue(registerSourceIndex) + instruction.getSignExtendedImmediateValue(), getRegisterValue(registerTargetIndex));
	return INSTRUCTION_TYPE_SW;
}


//--------------------------------------------------------------
// TODO : to implement the Shift Left Logical function (SLL).
// It should do left shift of target register value.
//
// Rust:
// fn op_sll(&mut self, instruction: Instruction) {
//     let i = instruction.shift(); load shift immediate value
//     let t = instruction.t();     load target register
//     let d = instruction.d();     load destination register
//
//     let v = self.reg(t) << i;
//
//     self.set_reg(d, v);
// }
//---------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeSLL(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_SLL;
}

//--------------------------------------------------------------
// TODO : to implement the Add Immediate Unsigned function (ADDIU).
// It should add source register value and sign extended immediate value.
//
// Rust:
// fn op_addiu(&mut self, instruction: Instruction) {
//     let i = instruction.imm_se(); load sign extended immediate value
//     let t = instruction.t();      load target register
//     let s = instruction.s();      load source register
//
//     let v = self.reg(s).wrapping_add(i);
//     self.set_reg(t, v);
// }
//----------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeADDIU(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_ADDIU;
}

//--------------------------------------------------------------
// TODO : to implement the Jump function (J).
// It should change the value of the PC register and perform jumping
// of the CPU execution pipeline to some other location in the memory.
//
// Rust:
// fn op_j(&mut self, instruction: Instruction) {
//     let i = instruction.imm_jump(); load jump target value
//
//     self.pc = (self.pc & 0xf0000000) | (i << 2);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeJ(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_J;
}

Cpu::InstructionType Cpu::opcodeOR(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(instruction.getRegisterSourceIndex()) | getRegisterValue(instruction.getRegisterTargetIndex()));
	return INSTRUCTION_TYPE_OR;
}

Cpu::InstructionType Cpu::opcodeCOP0(const Instruction& instruction)
{
	InstructionType instructionType = INSTRUCTION_TYPE_UNKNOWN;

	switch (instruction.getCopOpcodeValue())
	{
	case 0b00100:
		instructionType = opcodeMTC0(instruction);
		break;
	default:
		LOG("Unhandled instruction 0x" << std::hex << instruction.getCopOpcodeValue());
	}

	return instructionType;
}

Cpu::InstructionType Cpu::opcodeMTC0(const Instruction& instruction)
{
	uint32_t cop0Register = instruction.getRegisterDestinationIndex().m_index;
	uint32_t cop0RegisterValue = getRegisterValue(cop0Register);

	switch (cop0Register)
	{
	// $cop0_3 BPC, used to generate a breakpoint exception when the PC takes the given value
	case 3:
	// $cop0_5 BDA, the data breakpoint. It breaks when a certain address is accessed on a data load/store instead of a PC value
	case 5:
	// $cop0_6
	case 6:
	// $cop0_7 DCIC, used to enable and disable the various hardware breakpoints
	case 7:
	// $cop0_9 BDAM, it's a bitmask applied when testing BDA above
	case 9:
	// $cop0_11 BPCM, like BDAM but for masking the BPC breakpoint
	case 11:
		// Breakpoints registers
		if (cop0RegisterValue)
			LOG("Unhandled write to cop0 register 0x" << std::hex << cop0Register);
		break;
	// $cop0_12
	case 12:
		// Status register, it's used to query and mask the exceptions and controlling the cache behaviour
		m_sr = getRegisterValue(instruction.getRegisterTargetIndex());
		break;
	// $cop0_13 CAUSE, which contains mostly read-only data describing the cause of an exception
	case 13:
		// Cause register
		if (cop0RegisterValue)
			LOG("Unhandled write to CAUSE register " << std::hex << cop0Register);
		break;
	default:
		LOG("Unhandled cop0 register 0x" << std::hex << cop0Register);
	}

	return INSTRUCTION_TYPE_MTC0;
}

// Branch to immediate value "offset"
void branch(uint32_t offset, uint32_t& pc)
{
	// Offset immediates are always shifted two places to the right
	// since "PC" addresses have to be aligned on 32 bits at all times.
	pc += (offset << 2);

	// We need to compensate for the hardcoded "pc += 4" in the runNextInstruction
	pc -= 4;
}

//--------------------------------------------------------------
// TODO : to implement the Branch If Not Equal function (BNE).
// It should compare values that are stored in source and tagret
// registers and if they are unequal, immediate value should be 
// added/substracted from the PC register.
//
// Rust:
// fn op_bne(&mut self, instruction: Instruction) {
//     let i = instruction.imm_se(); load sign extended immediate value
//     let s = instruction.s();      load source register
//     let t = instruction.t();      load target register
//
//     if self.reg(s) != self.reg(t) {
//         self.branch(i);
//     }
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeBNE(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_BNE;
}

Cpu::InstructionType Cpu::opcodeADDI(const Instruction& instruction)
{
	int32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex  = instruction.getRegisterSourceIndex();

	int32_t registerSourceValue = getRegisterValue(registerSourceIndex);

	assert(!(signExtendedImmediateValue > 0 && registerSourceValue > INT_MAX - signExtendedImmediateValue), "ADDI overflow");
	assert(!(signExtendedImmediateValue < 0 && registerSourceValue < INT_MIN - signExtendedImmediateValue), "ADDI underflow");

	setRegisterValue(instruction.getRegisterTargetIndex(), uint32_t(registerSourceValue + signExtendedImmediateValue));

	return INSTRUCTION_TYPE_ADDI;
}

//--------------------------------------------------------------
// TODO : to implement the Load Word function (LW).
// It should load 32 bit instruction from the memory.
//
// Rust:
// op_lw(&mut self, instruction: Instruction) {
//     if self.sr & 0x10000 != 0 {
//         Cache is isolated, ignore write
//         LOG("Ignoring load while cache is isolated");
//         ret CACHE_ISOLATED;
//     }
//
//     let i = instruction.imm_se(); load sign extended immediate value
//     let t = instruction.t();      load target register
//     let s = instruction.s();      load source register
//
//     let addr = self.reg(s).wrapping_add(i);
//
//     let v = self.load32(addr);
//
//     let status = v.status;
//     if (status == UNALIGNED_ACCESS || status == UNHANDLED_FETCH)
//         ret INSTRUCTION_TYPE_UNKNOWN;
//     if (status == NOT_IMPLEMENTED)
//         ret INSTRUCTION_TYPE_NOT_IMPLEMENTED;
//
//     Put the load in the delay slot
//     self.load = RegisterData(t, v.instruction);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeLW(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_LW;
}

Cpu::InstructionType Cpu::opcodeSLTU(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t registerValueComparison = getRegisterValue(registerSourceIndex) < getRegisterValue(registerTargetIndex);

	setRegisterValue(instruction.getRegisterDestinationIndex(), registerValueComparison);
	return INSTRUCTION_TYPE_SLTU;
}

Cpu::InstructionType Cpu::opcodeADDU(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(registerSourceIndex) + getRegisterValue(registerTargetIndex));
	return INSTRUCTION_TYPE_ADDU;
}

//--------------------------------------------------------------
// TODO : to implement the Store Halfword function (SH).
// It should store 16 bit value into the memory.
//
// Rust:
// fn op_sh(&mut self, instruction: Instruction) {
//     if self.sr & 0x10000 != 0 {
//         LOG("Ignoring store while cache is isolated");
//         return CACHE_ISOLATED;
//     }
//
//     let i = instruction.imm_se(); load sign extended immediate value
//     let t = instruction.t();      load target register
//     let s = instruction.s();      load source register
//
//     let addr = self.reg(s).wrapping_add(i);
//     let v = self.reg(t);
//
//     self.store16(addr, v as u16);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeSH(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_SH;
}

//--------------------------------------------------------------
// TODO : to implement the Jump And Link function (JAL).
// It should behave like a regular jump instruction except that it also
// stores the return address in $ra ($31).
//
// Rust:
// fn op_jal(&mut self, instruction: Instruction) {
//     let ra = self.pc;
//
//     Store return address in $31 ($ra)
//     self.set_reg(RegisterIndex(31), ra);
//
//     Jump instruction
//     self.op_j(instruction);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeJAL(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_JAL;
}

Cpu::InstructionType Cpu::opcodeANDI(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	setRegisterValue(registerTargetIndex, getRegisterValue(registerSourceIndex) & instruction.getImmediateValue());
	return INSTRUCTION_TYPE_ANDI;
}

//--------------------------------------------------------------
// TODO : to implement the Store Byte function (SB).
// It should store 8 bit value into the memory.
//
// Rust:
// fn op_sb(&mut self, instruction: Instruction) {
//     if self.sr & 0x10000 != 0 {
//         LOG("Ignoring store while cache is isolated");
//         return CACHE_ISOLATED;
//     }
//
//     let i = instruction.imm_se(); load sign extended immediate value
//     let t = instruction.t();      load target register
//     let s = instruction.s();      load source register
//
//     let addr = self.reg(s).wrapping_add(i);
//     let v = self.reg(t);
//
//     self.store8(addr, v as u8);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeSB(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_SB;
}

//--------------------------------------------------------------
// TODO : to implement the Jump Register function (JR).
// It should set PC to the value stored in one of general purpose registers.
//
// Rust:
// fn op_jr(&mut self, instruction: Instruction) {
//     let s = instruction.s(); load source register
//
//     self.pc = self.reg(s);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeJR(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_JR;
}

//--------------------------------------------------------------
// TODO : to implement the Load Byte (Signed) function (LB).
// It should load 8 bit value from the memory.
//
// Rust:
// fn op_lb(&mut self, instruction: Instruction) {
//     let i = instruction.imm_se(); load sign extended immediate value
//     let t = instruction.t();      load target register
//     let s = instruction.s();      load source register
//
//     let addr = self.reg(s).wrapping_add(i);
//
//     Cast as i8 to force sign extension
//     let v = self.load8(addr) as i8;
//
//     let status = v.status;
//     if (status == UNALIGNED_ACCESS || status == UNHANDLED_FETCH)
//         ret INSTRUCTION_TYPE_UNKNOWN;
//     if (status == NOT_IMPLEMENTED)
//         ret INSTRUCTION_TYPE_NOT_IMPLEMENTED;
//
//     self.load = RegisterData(t, v as u32);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeLB(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_LB;
}

//--------------------------------------------------------------
// TODO : to implement the Branch If Equal function (BEQ).
// It should compare values that are stored in source and tagret
// registers and if they are equal, immediate value should be 
// added/substracted from the PC register. 
//
// Rust:
// fn op_beq(&mut self, instruction: Instruction) {
//     let i = instruction.imm_se(); load sign extended immediate value
//     let s = instruction.s();      load source register
//     let t = instruction.t();      load target register
//
//     if self.reg(s) == self.reg(t) {
//         self.branch(i);
//     }
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeBEQ(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_BEQ;
}

uint32_t Cpu::getRegisterValue(RegisterIndex registerIndex) const
{
	assert(registerIndex.m_index < _countof(m_regs), "Index out of boundaries");
	return m_regs[registerIndex.m_index];
}

void Cpu::setRegisterValue(RegisterIndex registerIndex, uint32_t value)
{
	assert(registerIndex.m_index < _countof(m_outRegs), "Index out of boundaries");
	if (registerIndex.m_index > 0) m_outRegs[registerIndex.m_index] = value;
}