#include "pscx_cpu.h"

#include <iostream>
#include <cassert>
#include <climits>

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

Instruction Cpu::load16(uint32_t addr) const
{
	return m_inter.load16(addr);
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
		case/*AND*/0b100100:
			instructionType = opcodeAND(instruction);
			break;
		case/*ADD*/0b100000:
			instructionType = opcodeADD(instruction);
			break;
		case/*JALR*/0b001001:
			instructionType = opcodeJALR(instruction);
			break;
		case/*SUBU*/0b100011:
			instructionType = opcodeSUBU(instruction);
			break;
		case/*SRA*/0b000011:
			instructionType = opcodeSRA(instruction);
			break;
		case/*DIV*/0b011010:
			instructionType = opcodeDIV(instruction);
			break;
		case/*MFLO*/0b010010:
			instructionType = opcodeMFLO(instruction);
			break;
		case/*SRL*/0b000010:
			instructionType = opcodeSRL(instruction);
			break;
		case/*DIVU*/0b011011:
			instructionType = opcodeDIVU(instruction);
			break;
		case/*MFHI*/0b010000:
			instructionType = opcodeMFHI(instruction);
			break;
		case/*SLT*/0b101010:
			instructionType = opcodeSLT(instruction);
			break;
		case/*SYSCALL*/0b001100:
			instructionType = opcodeSYSCALL(instruction);
			break;
		case/*MTLO*/0b010011:
			instructionType = opcodeMTLO(instruction);
			break;
		case/*MTHI*/0b010001:
			instructionType = opcodeMTHI(instruction);
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
	case/*BGTZ*/0b000111:
		instructionType = opcodeBGTZ(instruction);
		break;
	case/*BLEZ*/0b000110:
		instructionType = opcodeBLEZ(instruction);
		break;
	case/*LBU*/0b100100:
		instructionType = opcodeLBU(instruction);
		break;
	case/*BXX*/0b000001:
		instructionType = opcodeBXX(instruction);
		break;
	case/*SLTI*/0b001010:
		instructionType = opcodeSLTI(instruction);
		break;
	case/*SLTIU*/0b001011:
		instructionType = opcodeSLTIU(instruction);
		break;
	case/*LHU*/0b100101:
		instructionType = opcodeLHU(instruction);
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
//     Save the address of the current instruction to save 'EPC' in the case of exception
//     self.current_pc = self.pc;
//
//     if (self.current_pc % 4 != 0) {
//         self.exception(Exception::LoadAddressError);
//         ret;
//     }
//
//     Fetch instruction at PC
//     let instruction = Instruction(self.load32(self.pc));
//
//     if (next_instruction.status == UNALIGNED_ACCESS ||
//         next_instruction.status == UNHANDLED_FETCH)
//         ret INSTRUCTION_TYPE_UNKNOWN;
//
//     Increment PC to point to the next instruction.
//     self.pc = self.next_pc;
//     self.next_pc = self.next_pc.wrapping_add(4);
//
//     If the last instruction was a branch then we're in the delay slot
//     self.delay_slot = self.branch;
//     self.branch = false;
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

	uint32_t addr = getRegisterValue(registerSourceIndex) + instruction.getSignExtendedImmediateValue();

	// Address must be 32 bit aligned
	if (addr % 4 == 0)
	{
		store32(addr, getRegisterValue(registerTargetIndex));
	}
	else
	{
		exception(Exception::EXCEPTION_STORE_ADDRESS_ERROR);
		return INSTRUCTION_TYPE_UNALIGNED;
	}

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
//     self.next_pc = (self.pc & 0xf0000000) | (i << 2);
//     self.branch = true;
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

//--------------------------------------------------------------
// TODO : to implement the Bitwise And function (AND).
// It should do bitwise and operation.
//
// Rust:
// fn op_and(&mut self, instruction: Instruction) {
//     let d = instruction.d();
//     let s = instruction.s();
//     let t = instruction.t();
//
//     let v = self.reg(s) & self.reg(t);
//
//     self.set_reg(d, v);
// }
//---------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeAND(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_AND;
}

Cpu::InstructionType Cpu::opcodeCOP0(const Instruction& instruction)
{
	InstructionType instructionType = INSTRUCTION_TYPE_UNKNOWN;

	switch (instruction.getCopOpcodeValue())
	{
	case/*MFC0*/0b00000:
		instructionType = opcodeMFC0(instruction);
		break;
	case/*MTC0*/0b00100:
		instructionType = opcodeMTC0(instruction);
		break;
	case/*RFE*/0b10000:
		instructionType = opcodeRFE(instruction);
		break;
	default:
		LOG("Unhandled instruction 0x" << std::hex << instruction.getCopOpcodeValue());
	}

	return instructionType;
}

Cpu::InstructionType Cpu::opcodeMTC0(const Instruction& instruction)
{
	uint32_t cop0Register = instruction.getRegisterDestinationIndex().getRegisterIndex();
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

//--------------------------------------------------------------
// TODO : to implement the Move From Coprocessor 0 function (MFC0)
// It should move instruction register value from coprocessor 0.
//
// Rust:
// fn op_mfc0(&mut self, instruction: Instruction) {
//     let cpu_r = instruction.t();
//     let cop_r = instruction.d().getRegisterIndex();
//
//     let v = match cop_r {
//         12 => self.sr,
//         13 => self.cause,
//         14 => self.epc,
//         -  => panic!("Unhandled read from cop0r{}", cop0_r),
//     };
//
//     self.load = (cpu_r, v);
// }
//----------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeMFC0(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_MFC0;
}

// Branch to immediate value "offset"
void Cpu::branch(uint32_t offset)
{
	// Offset immediates are always shifted two places to the right
	// since "PC" addresses have to be aligned on 32 bits at all times.
	m_nextPc = m_pc + (offset << 2);
	m_branch = true;
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

	if ((signExtendedImmediateValue > 0 && registerSourceValue > INT_MAX - signExtendedImmediateValue) ||
		(signExtendedImmediateValue < 0 && registerSourceValue < INT_MIN - signExtendedImmediateValue))
	{
		LOG("ADDI overflow");
		exception(Exception::EXCEPTION_OVERFLOW);
		return INSTRUCTION_TYPE_OVERFLOW;
	}
	else
	{
		setRegisterValue(instruction.getRegisterTargetIndex(), uint32_t(registerSourceValue + signExtendedImmediateValue));
	}

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
//     Address must be 32 bit aligned
//     if (addr % 4 == 0) {
//         let v = self.load32(addr);
//
//         let status = v.status;
//         if (status == UNALIGNED_ACCESS || status == UNHANDLED_FETCH)
//             ret INSTRUCTION_TYPE_UNKNOWN;
//         if (status == NOT_IMPLEMENTED)
//             ret INSTRUCTION_TYPE_NOT_IMPLEMENTED;
//
//         Put the load in the delay slot
//         self.load = RegisterData(t, v.instruction);
//     }
//     else {
//         self.exception(Exception::LoadAccessError);
//     }
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
//
//     Address must be 16 bit aligned
//     if (addr % 2 == 0) {
//         let v = self.reg(t);
//
//         self.store16(addr, v as u16);
//     }
//     else {
//         self.exception(Exception::StoreAddressError);
//     }
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
//
//     self.branch = true;
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
//     self.next_pc = self.reg(s);
//     self.branch = true;
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

Cpu::InstructionType Cpu::opcodeADD(const Instruction& instruction)
{
	int32_t registerSourceValue = getRegisterValue(instruction.getRegisterSourceIndex());
	int32_t registerTargetValue = getRegisterValue(instruction.getRegisterTargetIndex());

	if ((registerSourceValue > 0 && registerTargetValue > INT_MAX - registerSourceValue) ||
		(registerSourceValue < 0 && registerTargetValue < INT_MIN - registerSourceValue))
	{
		LOG("ADD overflow");
		exception(Exception::EXCEPTION_OVERFLOW);
		return INSTRUCTION_TYPE_OVERFLOW;
	}
	else
	{
		setRegisterValue(instruction.getRegisterDestinationIndex(), uint32_t(registerSourceValue + registerTargetValue));
	}

	return INSTRUCTION_TYPE_ADD;
}

//--------------------------------------------------------------
// TODO : to implement the Branch If Greater Than Zero function (BGTZ).
// It should compare a single general purpose register to 0 and if it is greater than 0,
// immediate value should be added/substracted from the PC register.
//
// Rust:
// fn op_bgtz(&mut self, instruction: Instruction) {
//     let i = instruction.imm_se();
//     let s = instruction.s();
//
//     let v = self.reg(s) as i32;
//
//     if v > 0 {
//         self.branch(i);
//     }
// }
//----------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeBGTZ(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_BGTZ;
}

//--------------------------------------------------------------
// TODO : to implement the Branch If Less Than Or Equal To Zero function (BLEZ).
// It should compare a signle general purpose register to 0 and if it is less than or equal to zero,
// immediate value should be added/substracted from the PC register.
//
// Rust:
// fn op_blez(&mut self, instruction: Instruction) {
//     let i = instruction.imm_se();
//     let s = instruction.s();
//
//     let v = self.reg(s) as i32;
//
//     if v <= 0 {
//         self.branch(i);
//     }
// }
//----------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeBLEZ(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_BLEZ;
}

Cpu::InstructionType Cpu::opcodeLBU(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + instruction.getSignExtendedImmediateValue();

	Instruction instructionLoaded = load8(addr);
	Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	m_load = RegisterData(registerTargetIndex, instructionLoaded.getInstructionOpcode());

	return INSTRUCTION_TYPE_LBU;
}

//--------------------------------------------------------------
// TODO : to implement the Jump And Link Register function (JALR).
// It should set PC register to the value stored in one of general purpose registers
// and store the return address in the general purpose register.
//
// Rust:
// fn op_jarl(&mut self, instruction: Instruction) {
//     let d = instruction.d();
//     let s = instruction.s();
//
//     let ra = self.next_pc;
//
//     Store return address in 'd'
//     self.set_reg(d, ra);
//
//     self.next_pc = self.reg(s);
//
//     self.branch = true;
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeJALR(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_JALR;
}

Cpu::InstructionType Cpu::opcodeBXX(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();

	uint32_t instructionOpcode = instruction.getInstructionOpcode();

	uint32_t isBGEZ = (instructionOpcode >> 16) & 1;
	bool isLink = (instructionOpcode >> 17) & 0xf == 8;

	int32_t sourceValue = getRegisterValue(registerSourceIndex);

	uint32_t test = (sourceValue < 0);

	test ^= isBGEZ;

	if (isLink)
	{
		uint32_t ra = m_nextPc;
		setRegisterValue(RegisterIndex(31), ra);
	}

	if (test)
	{
		branch(signExtendedImmediateValue);
	}
	
	return INSTRUCTION_TYPE_BXX;
}

//--------------------------------------------------------------
// TODO : to implement the Set If Less Than Immediate function (SLTI).
// It should compare register with an immediate value (sign-extended).
// Comparison is done by using signed arithmetics.
//
// Rust:
// fn op_slti(&mut self, instruction: Instruction) {
//     let i = instruction.imm_se() as i32;
//     let s = instruction.s();
//     let t = instruction.t();
//
//     let v = (self.reg(s) as i32) < i;
//
//     self.set_reg(t, v as u32);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeSLTI(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_SLTI;
}

//--------------------------------------------------------------
// TODO : to implement the Substract Unsigned function (SUBU).
// It should substract target register value from source register value.
//
// Rust:
// fn op_subu(&mut self, instruction: Instruction) {
//     let s = instruction.s();
//     let t = instruction.t();
//     let d = instruction.d();
//
//     let v = self.reg(s).wrapping_sub(self.reg(t));
//
//     self.set_reg(d, v);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeSUBU(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_SUBU;
}

//--------------------------------------------------------------
// TODO : to implement the Shift Right Arithmetic function (SRA).
// It should implement arithmetic shift right instruction.
//
// Rust:
// fn op_sra(&mut self, instruction: Instruction) {
//     let i = instruction.shift();
//     let t = instruction.t();
//     let d = instruction.d();
//
//     let v = (self.reg(t) as i32) >> i;
//
//     self.set_reg(d, v as u32);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeSRA(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_SRA;
}

Cpu::InstructionType Cpu::opcodeDIV(const Instruction& instruction)
{
	int32_t numerator = getRegisterValue(instruction.getRegisterSourceIndex());
	int32_t denominator = getRegisterValue(instruction.getRegisterTargetIndex());

	if (denominator == 0)
	{
		// Division by zero, results are bogus
		m_hi = numerator;
		m_lo = numerator >= 0 ? 0xffffffff : 0x1;
	}
	else if ((uint32_t)numerator == 0x80000000 && denominator == -1)
	{
		// Result is not representable in a 32 bit signed integer
		m_hi = 0x0;
		m_lo = 0x80000000;
	}
	else
	{
		m_hi = numerator % denominator;
		m_lo = numerator / denominator;
	}

	return INSTRUCTION_TYPE_DIV;
}

//--------------------------------------------------------------
// TODO : to implement the Move From LO function (MFLO).
// It should move the contents of LO in a general purpose register.
//
// Rust:
// fn op_mflo(&mut self, instruction: Instruction) {
//     let d = instruction.d();
//
//     let lo = self.lo;
//
//     self.set_reg(d, lo);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeMFLO(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_MFLO;
}

//--------------------------------------------------------------
// TODO : to implement the Shift Right Logical function (SRL).
// It should implement right logical shift.
//
// Rust:
// fn op_srl(&mut self, instruction: Instruction) {
//     let i = instruction.shift();
//     let t = instruction.t();
//     let d = instruction.d();
//
//     let v = self.reg(t) >> i;
//
//     self.set_reg(d, v);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeSRL(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_SRL;
}

//--------------------------------------------------------------
// TODO : to implement the Set If Less Than Immediate Unsigned function (SLTIU).
// It should compare register with an immediate value (sign-extended).
// Comparison is done by using unsigned arithmetics.
//
// Rust:
// fn op_sltiu(&mut self, instruction: Instruction) {
//     let i = instruction.imm_se();
//     let s = instruction.s();
//     let t = instruction.t();
//
//     let v = self.reg(s) < i;
//
//     self.set_reg(t, v as u32);
// }
//--------------------------------------------------------------
Cpu::InstructionType Cpu::opcodeSLTIU(const Instruction& instruction)
{
	// Fixme
	return INSTRUCTION_TYPE_SLTIU;
}

Cpu::InstructionType Cpu::opcodeDIVU(const Instruction& instruction)
{
	uint32_t numerator = getRegisterValue(instruction.getRegisterSourceIndex());
	uint32_t denominator = getRegisterValue(instruction.getRegisterTargetIndex());

	if (denominator == 0)
	{
		// Division by zero, results are bogus
		m_hi = numerator;
		m_lo = 0xffffffff;
	}
	else
	{
		m_hi = numerator % denominator;
		m_lo = numerator / denominator;
	}

	return INSTRUCTION_TYPE_DIVU;
}

Cpu::InstructionType Cpu::opcodeSLT(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterDestinationIndex(), (int32_t)getRegisterValue(instruction.getRegisterSourceIndex()) < (int32_t)getRegisterValue(instruction.getRegisterTargetIndex()));
	return INSTRUCTION_TYPE_SLT;
}

Cpu::InstructionType Cpu::opcodeMFHI(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterDestinationIndex(), m_hi);
	return INSTRUCTION_TYPE_MFHI;
}

void Cpu::exception(Exception cause)
{
	// Exception handler address depends on the 'BEV' bit:
	uint32_t handler = m_sr & (1 << 22) ? 0xbfc00180 : 0x80000080;

	// Shift bits [5:0] of 'SR' two places to the left.
	// Those bits are three pairs of Interrupt Enable/User
	// Mode bits behaving like a stack 3 entries deep.
	// Entering an exception pushes a pair of zeros
	// by left shifting the stack which disables
	// interrupts and puts the CPU in kernel mode.
	// The original third entry is discarded ( it's up
	// to the kernel to handle more than two recursive
	// exception levels ).
	uint32_t mode = m_sr & 0x3f;
	m_sr &= (~0x3f);
	m_sr |= (mode << 2) & 0x3f;

	// Update 'CAUSE' register with the exception code ( bits [6:2] )
	m_cause = ((uint32_t)cause) << 2;

	// Save current instruction address in 'EPC'
	m_epc = m_currentPc;

	if (m_delaySlot)
	{
		// When an exception occurs in a delay slot 'EPC' points
		// to the branch instruction and bit 31 of 'CAUSE' is set
		m_epc += 4;
		m_cause |= 1 << 31;
	}

	// Exceptions don't have a branch delay, we jump directly
	// into the handler
	m_pc = handler;
	m_nextPc = m_pc + 4;
}

Cpu::InstructionType Cpu::opcodeSYSCALL(const Instruction& instruction)
{
	exception(Exception::EXCEPTION_SYSCALL);
	return INSTRUCTION_TYPE_SYSCALL;
}

Cpu::InstructionType Cpu::opcodeMTLO(const Instruction& instruction)
{
	m_lo = getRegisterValue(instruction.getRegisterSourceIndex());
	return INSTRUCTION_TYPE_MTLO;
}

Cpu::InstructionType Cpu::opcodeMTHI(const Instruction& instruction)
{
	m_hi = getRegisterValue(instruction.getRegisterSourceIndex());
	return INSTRUCTION_TYPE_MTHI;
}

Cpu::InstructionType Cpu::opcodeRFE(const Instruction& instruction)
{
	// There are other instructions with the same encoding but all
	// are virtual memory related and the Playstation doesn't implement them.
	// Still, let's make sure we're not running buggy code.
	if (instruction.getInstructionOpcode() & 0x3f != 0b010000)
	{
		LOG("Invalid cop 0 instruction:" << std::hex << instruction.getInstructionOpcode());
		return INSTRUCTION_TYPE_UNKNOWN;
	}

	// Restore the pre-exception mode by shifting the Interrupt
	// Enable/User Mode stack back to its original position.
	uint32_t mode = m_sr & 0x3f;
	m_sr &= (!0x3f);
	m_sr |= mode >> 2;

	return INSTRUCTION_TYPE_RFE;
}

Cpu::InstructionType Cpu::opcodeLHU(const Instruction& instruction)
{
	uint32_t addr = getRegisterValue(instruction.getRegisterSourceIndex()) + instruction.getSignExtendedImmediateValue();

	// Address must be 16 bit aligned
	if (addr % 2 == 0)
	{
		Instruction instructionLoaded = load16(addr);
		m_load = RegisterData(instruction.getRegisterTargetIndex(), instructionLoaded.getInstructionOpcode());
	}
	else
	{
		exception(Exception::EXCEPTION_LOAD_ADDRESS_ERROR);
		return INSTRUCTION_TYPE_UNALIGNED;
	}

	return INSTRUCTION_TYPE_LHU;
}

uint32_t Cpu::getRegisterValue(RegisterIndex registerIndex) const
{
	assert(registerIndex.getRegisterIndex() < _countof(m_regs), "Index out of boundaries");
	return m_regs[registerIndex.getRegisterIndex()];
}

void Cpu::setRegisterValue(RegisterIndex registerIndex, uint32_t value)
{
	assert(registerIndex.getRegisterIndex() < _countof(m_outRegs), "Index out of boundaries");
	if (registerIndex.getRegisterIndex() > 0) m_outRegs[registerIndex.getRegisterIndex()] = value;
}