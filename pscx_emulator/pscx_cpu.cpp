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

std::vector<Profile*> Cpu::getPadProfiles()
{
	return m_inter.getPadProfiles();
}

template<typename T>
Instruction Cpu::load(uint32_t addr)
{
	return m_inter.load<T>(m_timeKeeper, addr);
}

template<typename T>
void Cpu::store(uint32_t addr, T value)
{
	if (m_cop0.isCacheIsolated())
		return cacheMaintenance<T>(addr, value);
	return m_inter.store<T>(m_timeKeeper, addr, value);
}

template<typename T>
void Cpu::cacheMaintenance(uint32_t addr, T value)
{
	// Implementing full cache emulation requires handling many
	// corner cases. Adding suppirt for cache invalidation which is the only
	// use case for cache isolation.
	CacheControl cacheControl = m_inter.getCacheControl();

	assert(("Cache maintenance while instruction cache is disabled", cacheControl.icacheEnabled()));
	assert(("Unsupported write while cache is isolated", (std::is_same<uint32_t, T>::value) && value == 0));

	uint32_t line = (addr >> 4) & 0xff;

	// Fetch the cacheline for this address
	ICacheLine& cacheLine = m_icache[line];
	if (cacheControl.tagTestMode())
	{
		// In tag test mode the write invalidates the entire
		// targeted cacheline
		cacheLine.invalidate();
	}
	else
	{
		// Otherwise the write ends up directly in the cache
		uint32_t index = (addr >> 2) & 3;

		Instruction instruction(value);
		cacheLine.setInstruction(index, instruction);
	}
}

// TODO: take a look how to get back error messages.
Cpu::InstructionType Cpu::decodeAndExecute(const Instruction& instruction)
{
	// Simulate instruction execution time.
	m_timeKeeper.tick(1);

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
		case/*SLLV*/0b000100:
			instructionType = opcodeSLLV(instruction);
			break;
		case/*NOR*/0b100111:
			instructionType = opcodeNOR(instruction);
			break;
		case/*SRAV*/0b000111:
			instructionType = opcodeSRAV(instruction);
			break;
		case/*SRLV*/0b000110:
			instructionType = opcodeSRLV(instruction);
			break;
		case/*MULTU*/0b011001:
			instructionType = opcodeMULTU(instruction);
			break;
		case/*XOR*/0b100110:
			instructionType = opcodeXOR(instruction);
			break;
		case/*BREAK*/0b001101:
			instructionType = opcodeBREAK(instruction);
			break;
		case/*MULT*/0b011000:
			instructionType = opcodeMULT(instruction);
			break;
		case/*SUB*/0b100010:
			instructionType = opcodeSUB(instruction);
			break;
		default/*Illegal instruction*/:
			instructionType = opcodeIllegal(instruction);
		}
		break;
	case /*LUI*/0b001111:
		instructionType = opcodeLUI(instruction);
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
	case/*LH*/0b100001:
		instructionType = opcodeLH(instruction);
		break;
	case/*XORI*/0b001110:
		instructionType = opcodeXORI(instruction);
		break;
	case/*COP1*/0b010001:
		instructionType = opcodeCOP1(instruction);
		break;
	case/*COP2*/0b010010:
		instructionType = opcodeCOP2(instruction);
		break;
	case/*COP3*/0b010011:
		instructionType = opcodeCOP3(instruction);
		break;
	case/*LWL*/0b100010:
		instructionType = opcodeLWL(instruction);
		break;
	case/*LWR*/0b100110:
		instructionType = opcodeLWR(instruction);
		break;
	case/*SWL*/0b101010:
		instructionType = opcodeSWL(instruction);
		break;
	case/*SWR*/0b101110:
		instructionType = opcodeSWR(instruction);
		break;
	case/*LWC0*/0b110000:
		instructionType = opcodeLWC0(instruction);
		break;
	case/*LWC1*/0b110001:
		instructionType = opcodeLWC1(instruction);
		break;
	case/*LWC2*/0b110010:
		instructionType = opcodeLWC2(instruction);
		break;
	case/*LWC3*/0b110011:
		instructionType = opcodeLWC3(instruction);
		break;
	case/*SWC0*/0b111000:
		instructionType = opcodeSWC0(instruction);
		break;
	case/*SWC1*/0b111001:
		instructionType = opcodeSWC1(instruction);
		break;
	case/*SWC2*/0b111010:
		instructionType = opcodeSWC2(instruction);
		break;
	case/*SWC3*/0b111011:
		instructionType = opcodeSWC3(instruction);
		break;
	default/*Illegal instruction*/:
		instructionType = opcodeIllegal(instruction);
	}

	//m_debugInstructions.push_back(instruction.getInstructionOpcode());
	return instructionType;
}

Cpu::InstructionType Cpu::runNextInstuction()
{
	// Synchronize the peripherals
	// m_inter.sync(m_timeKeeper);
	if (m_timeKeeper.syncPending())
	{
		m_inter.sync(m_timeKeeper);
		m_timeKeeper.updateSyncPending();
	}

	// Store the address of the current instruction to save in 'EPC' in the case of an exception.
	m_currentPc = m_pc;

	if (m_currentPc % 4 != 0)
	{
		// PC is not correctly aligned !
		exception(Exception::EXCEPTION_LOAD_ADDRESS_ERROR);
		return INSTRUCTION_TYPE_UNALIGNED;
	}

	// Fetch instruction at PC
	//Instruction instruction = load<uint32_t>(m_pc);
	Instruction instruction = fetchInstruction();
	Instruction::InstructionStatus instructionStatus = instruction.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;

	// Increment next PC to point to the next instruction.
	// All instructions are 32 bit long.
	m_pc = m_nextPc;
	m_nextPc += 4;

	// If the last instruction was a branch then we're in the delay slot
	m_delaySlot = m_branch;
	m_branch = false;

	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	// Execute the pending load ( if any, otherwise it will load $zero which is a NOP )
	// setRegisterValue works only on m_outRegs so this operation won't be visible by the next instruction
	RegisterData load = m_load;
	setRegisterValue(load.m_registerIndex, load.m_registerValue);

	// We reset the load to target register 0 for the next instruction
	m_load = RegisterData(RegisterIndex(0x0), 0x0);

	InstructionType instructionType = INSTRUCTION_TYPE_UNKNOWN;
	// Check for pending interrupts
	if (m_cop0.isIrqActive(m_inter.getIrqState()))
	{
		exception(Exception::EXCEPTION_INTERRUPT);
		instructionType = INSTRUCTION_TYPE_EXCEPTION_INTERRUPT;
	}
	else
	{
		// No interrupt pending, run the current instruction
		instructionType = decodeAndExecute(instruction);
	}

	// Copy the output registers as input for the next instruction
	memcpy(m_regs, m_outRegs, sizeof(m_regs));

	return instructionType;
}

Instruction Cpu::fetchInstruction()
{
	uint32_t pc = m_currentPc;
	CacheControl cacheControl = m_inter.getCacheControl();

	// KUSEG and KSEG0 regions are cached. KSEG1 is uncached and
	// KSEG2 doesn't contain any code.
	bool cached = (pc < 0xa0000000);

	if (cached && cacheControl.icacheEnabled())
	{
		// The MSB is ignored: running from KUSEG or KSEG0 hits
		// the same cachelines. So for instance addresses
		// 0x00000000 and 0x90000000 have the same tag and you can
		// jump from one to another without having to reload the cache.

		// Cache tag: bits [30:12]
		uint32_t tag = pc & 0x7ffff000;
		// Cache line "bucket": bits [11:4]
		uint32_t line = (pc >> 4) & 0xff;
		// Index in the cache line: bits [3:2]
		uint32_t index = (pc >> 2) & 3;

		// Fetch the cacheline for this address
		ICacheLine& cacheLine = m_icache[line];

		// Check the tag and validity
		if (cacheLine.getTag() != tag || cacheLine.getValidIndex() > index)
		{
			// Cache miss. Fetch the cacheline starting at the current index.
			// If the index is not 0 then some words are going to remain invalid in the cacheline.
			uint32_t currentPc = pc;

			// Fetching takes 3 cycles + 1 per instruction on
			// average.
			m_timeKeeper.tick(3);

			for (size_t i = index; i < 4; ++i)
			{
				m_timeKeeper.tick(1);

				Instruction instruction = m_inter.loadInstruction<uint32_t>(currentPc);
				cacheLine.setInstruction(static_cast<uint32_t>(i), instruction);
				currentPc += 4;
			}

			// Set the tag and valid bits
			cacheLine.setTagValid(pc);
		}

		// Cache line is now guaranteed to be valid
		return cacheLine.getInstruction(index);
	}
	// Cache is disabled, fetch directly from memory.
	// Takes 4 cycles on average.
	m_timeKeeper.tick(4);

	return m_inter.loadInstruction<uint32_t>(pc);
}

Cpu::InstructionType Cpu::opcodeLUI(const Instruction& instruction)
{
	// Low 16 bits are set to 0
	setRegisterValue(instruction.getRegisterTargetIndex(), instruction.getImmediateValue() << 16);
	return INSTRUCTION_TYPE_LUI;
}

Cpu::InstructionType Cpu::opcodeORI(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterTargetIndex(), instruction.getImmediateValue() | getRegisterValue(instruction.getRegisterSourceIndex()));
	return INSTRUCTION_TYPE_ORI;
}

Cpu::InstructionType Cpu::opcodeSW(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + instruction.getSignExtendedImmediateValue();

	// Address must be 32 bit aligned
	if (addr % 4 == 0)
	{
		store<uint32_t>(addr, getRegisterValue(registerTargetIndex));
	}
	else
	{
		exception(Exception::EXCEPTION_STORE_ADDRESS_ERROR);
		return INSTRUCTION_TYPE_UNALIGNED;
	}

	return INSTRUCTION_TYPE_SW;
}

Cpu::InstructionType Cpu::opcodeSLL(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(instruction.getRegisterTargetIndex()) << instruction.getShiftImmediateValue());
	return INSTRUCTION_TYPE_SLL;
}

Cpu::InstructionType Cpu::opcodeADDIU(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterTargetIndex(), getRegisterValue(instruction.getRegisterSourceIndex()) + instruction.getSignExtendedImmediateValue());
	return INSTRUCTION_TYPE_ADDIU;
}

Cpu::InstructionType Cpu::opcodeJ(const Instruction& instruction)
{
	m_nextPc = (m_pc & 0xf0000000) | (instruction.getJumpTargetValue() << 2);
	m_branch = true;
	return INSTRUCTION_TYPE_J;
}

Cpu::InstructionType Cpu::opcodeOR(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(instruction.getRegisterSourceIndex()) | getRegisterValue(instruction.getRegisterTargetIndex()));
	return INSTRUCTION_TYPE_OR;
}

Cpu::InstructionType Cpu::opcodeAND(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(instruction.getRegisterSourceIndex()) & getRegisterValue(instruction.getRegisterTargetIndex()));
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
	uint32_t targetRegisterValue = getRegisterValue(instruction.getRegisterTargetIndex());

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
		if (targetRegisterValue)
		{
			LOG("Unhandled write to cop0 register 0x" << std::hex << cop0Register);
		}
		break;
	// $cop0_12
	case 12:
		// Status register, it's used to query and mask the exceptions and controlling the cache behaviour
		m_cop0.setStatusRegister(targetRegisterValue);
		break;
	// $cop0_13 CAUSE, which contains mostly read-only data describing the cause of an exception
	case 13:
		// Cause register
		if (targetRegisterValue)
		{
			LOG("Unhandled write to CAUSE register " << std::hex << cop0Register);
		}
		break;
	default:
		LOG("Unhandled cop0 register 0x" << std::hex << cop0Register);
	}

	return INSTRUCTION_TYPE_MTC0;
}

Cpu::InstructionType Cpu::opcodeMFC0(const Instruction& instruction)
{
	uint32_t cop0Register     = instruction.getRegisterDestinationIndex().getRegisterIndex();
	RegisterIndex cpuRegister = instruction.getRegisterTargetIndex();

	switch (cop0Register)
	{
	case 12:
		// Load data only from $cop0_12 register
		m_load = RegisterData(cpuRegister, m_cop0.getStatusRegister());
		break;
	// $cop0_13 CAUSE
	case 13:
		m_load = RegisterData(cpuRegister, m_cop0.getCauseRegister(m_inter.getIrqState()));
		break;
	// $cop0_14 EPC
	case 14:
		m_load = RegisterData(cpuRegister, m_cop0.getExceptionPCRegister());
		break;
	default:
		LOG("Unhandled read from cop0Register 0x" << std::hex << cop0Register);
	}

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

Cpu::InstructionType Cpu::opcodeBNE(const Instruction& instruction)
{
	if (getRegisterValue(instruction.getRegisterSourceIndex()) != getRegisterValue(instruction.getRegisterTargetIndex()))
		branch(instruction.getSignExtendedImmediateValue());

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

Cpu::InstructionType Cpu::opcodeLW(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerTargetIndex   = instruction.getRegisterTargetIndex();
	RegisterIndex registerSourceIndex   = instruction.getRegisterSourceIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + signExtendedImmediateValue;

	// Address must be 32 bit aligned
	if (addr % 4 == 0)
	{
		Instruction instructionLoaded = load<uint32_t>(addr);
		Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
		if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
			instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
			return INSTRUCTION_TYPE_UNKNOWN;
		if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
			return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

		// Put the load in the delay slot
		m_load = RegisterData(registerTargetIndex, instructionLoaded.getInstructionOpcode());
	}
	else
	{
		exception(Exception::EXCEPTION_LOAD_ADDRESS_ERROR);
		return INSTRUCTION_TYPE_UNALIGNED;
	}

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

Cpu::InstructionType Cpu::opcodeSH(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + instruction.getSignExtendedImmediateValue();

	// Address must be 16 bit aligned
	if (addr % 2 == 0)
	{
		store<uint16_t>(addr, getRegisterValue(registerTargetIndex));
	}
	else
	{
		exception(Exception::EXCEPTION_STORE_ADDRESS_ERROR);
		return INSTRUCTION_TYPE_UNALIGNED;
	}

	return INSTRUCTION_TYPE_SH;
}

Cpu::InstructionType Cpu::opcodeJAL(const Instruction& instruction)
{
	//uint32_t ra = m_pc;
	uint32_t ra = m_nextPc;

	// Store return address in $31 ($ra)
	setRegisterValue(RegisterIndex(31), ra);

	// Jump instruction
	opcodeJ(instruction);

	m_branch = true;

	return INSTRUCTION_TYPE_JAL;
}

Cpu::InstructionType Cpu::opcodeANDI(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	setRegisterValue(registerTargetIndex, getRegisterValue(registerSourceIndex) & instruction.getImmediateValue());
	return INSTRUCTION_TYPE_ANDI;
}

Cpu::InstructionType Cpu::opcodeSB(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + instruction.getSignExtendedImmediateValue();

	store<uint8_t>(addr, getRegisterValue(registerTargetIndex));

	return INSTRUCTION_TYPE_SB;
}

Cpu::InstructionType Cpu::opcodeJR(const Instruction& instruction)
{
	//m_pc = getRegisterValue(instruction.getRegisterSourceIndex());
	m_nextPc = getRegisterValue(instruction.getRegisterSourceIndex());
	m_branch = true;
	return INSTRUCTION_TYPE_JR;
}

Cpu::InstructionType Cpu::opcodeLB(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + instruction.getSignExtendedImmediateValue();

	Instruction instructionLoaded = load<uint8_t>(addr);
	Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	int8_t register8Bit = (uint8_t)instructionLoaded.getInstructionOpcode();
	m_load = RegisterData(registerTargetIndex, (uint32_t)register8Bit);

	return INSTRUCTION_TYPE_LB;
}

Cpu::InstructionType Cpu::opcodeBEQ(const Instruction& instruction)
{
	if (getRegisterValue(instruction.getRegisterSourceIndex()) == getRegisterValue(instruction.getRegisterTargetIndex()))
		branch(instruction.getSignExtendedImmediateValue());

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

Cpu::InstructionType Cpu::opcodeBGTZ(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();

	int32_t registerSourceValue = getRegisterValue(registerSourceIndex);
	if (registerSourceValue > 0)
		branch(signExtendedImmediateValue);

	return INSTRUCTION_TYPE_BGTZ;
}

Cpu::InstructionType Cpu::opcodeBLEZ(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();

	int32_t registerSourceValue = getRegisterValue(registerSourceIndex);
	if (registerSourceValue <= 0)
		branch(signExtendedImmediateValue);

	return INSTRUCTION_TYPE_BLEZ;
}

Cpu::InstructionType Cpu::opcodeLBU(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + instruction.getSignExtendedImmediateValue();

	Instruction instructionLoaded = load<uint8_t>(addr);
	Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	m_load = RegisterData(registerTargetIndex, instructionLoaded.getInstructionOpcode());

	return INSTRUCTION_TYPE_LBU;
}

Cpu::InstructionType Cpu::opcodeJALR(const Instruction& instruction)
{
	uint32_t ra = m_nextPc;

	// Store return address in register destination index
	setRegisterValue(instruction.getRegisterDestinationIndex(), ra);

	m_nextPc = getRegisterValue(instruction.getRegisterSourceIndex());

	m_branch = true;

	return INSTRUCTION_TYPE_JALR;
}

Cpu::InstructionType Cpu::opcodeBXX(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();

	uint32_t instructionOpcode = instruction.getInstructionOpcode();

	uint32_t isBGEZ = (instructionOpcode >> 16) & 1;
	bool isLink = ((instructionOpcode >> 17) & 0xf) == 8;

	int32_t sourceValue = getRegisterValue(registerSourceIndex);

	uint32_t test = (sourceValue < 0);

	test ^= isBGEZ;

	if (isLink)
	{
		uint32_t ra = m_nextPc;
		setRegisterValue(RegisterIndex(31), ra);
	}

	if (test != 0)
	{
		branch(signExtendedImmediateValue);
	}
	
	return INSTRUCTION_TYPE_BXX;
}

Cpu::InstructionType Cpu::opcodeSLTI(const Instruction& instruction)
{
	int32_t registerSourceValue = getRegisterValue(instruction.getRegisterSourceIndex());
	int32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();

	setRegisterValue(instruction.getRegisterTargetIndex(), registerSourceValue < signExtendedImmediateValue);

	return INSTRUCTION_TYPE_SLTI;
}

Cpu::InstructionType Cpu::opcodeSUBU(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(registerSourceIndex) - getRegisterValue(registerTargetIndex));

	return INSTRUCTION_TYPE_SUBU;
}

Cpu::InstructionType Cpu::opcodeSRA(const Instruction& instruction)
{
	int32_t rightShiftedValue = ((int32_t)getRegisterValue(instruction.getRegisterTargetIndex())) >> instruction.getShiftImmediateValue();

	setRegisterValue(instruction.getRegisterDestinationIndex(), (uint32_t)rightShiftedValue);

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

Cpu::InstructionType Cpu::opcodeMFLO(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterDestinationIndex(), m_lo);
	return INSTRUCTION_TYPE_MFLO;
}

Cpu::InstructionType Cpu::opcodeSRL(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(instruction.getRegisterTargetIndex()) >> instruction.getShiftImmediateValue());
	return INSTRUCTION_TYPE_SRL;
}

Cpu::InstructionType Cpu::opcodeSLTIU(const Instruction& instruction)
{
	setRegisterValue(instruction.getRegisterTargetIndex(), getRegisterValue(instruction.getRegisterSourceIndex()) < instruction.getSignExtendedImmediateValue());
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
	uint32_t handlerAddr = m_cop0.enterException(cause, m_currentPc, m_delaySlot);

	// Exceptions don't have a branch delay, we jump directly
	// into the handler
	m_pc = handlerAddr;
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
	if ((instruction.getInstructionOpcode() & 0x3f) != 0b010000)
	{
		LOG("Invalid cop 0 instruction:" << std::hex << instruction.getInstructionOpcode());
		return INSTRUCTION_TYPE_UNKNOWN;
	}

	m_cop0.returnFromException();

	return INSTRUCTION_TYPE_RFE;
}

Cpu::InstructionType Cpu::opcodeLHU(const Instruction& instruction)
{
	uint32_t addr = getRegisterValue(instruction.getRegisterSourceIndex()) + instruction.getSignExtendedImmediateValue();

	// Address must be 16 bit aligned
	if (addr % 2 == 0)
	{
		Instruction instructionLoaded = load<uint16_t>(addr);
		m_load = RegisterData(instruction.getRegisterTargetIndex(), instructionLoaded.getInstructionOpcode());
	}
	else
	{
		exception(Exception::EXCEPTION_LOAD_ADDRESS_ERROR);
		return INSTRUCTION_TYPE_UNALIGNED;
	}

	return INSTRUCTION_TYPE_LHU;
}

Cpu::InstructionType Cpu::opcodeSLLV(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	// Shift amount is truncated to 5 bits
	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(registerTargetIndex) << (getRegisterValue(registerSourceIndex) & 0x1f));
	return INSTRUCTION_TYPE_SLLV;
}

Cpu::InstructionType Cpu::opcodeLH(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + signExtendedImmediateValue;

	// Cast as i16 to force sign extension
	Instruction instructionLoaded = load<uint16_t>(addr);
	Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	int16_t instructionOpcode = instructionLoaded.getInstructionOpcode();

	// Put the load in the delay slot
	m_load = RegisterData(registerTargetIndex, instructionOpcode);
	return INSTRUCTION_TYPE_LH;
}

Cpu::InstructionType Cpu::opcodeNOR(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	setRegisterValue(instruction.getRegisterDestinationIndex(), ~(getRegisterValue(registerSourceIndex) | getRegisterValue(registerTargetIndex)));
	return INSTRUCTION_TYPE_NOR;
}

Cpu::InstructionType Cpu::opcodeSRAV(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	// Shift amount is truncated to 5 bits
	setRegisterValue(instruction.getRegisterDestinationIndex(), ((int32_t)getRegisterValue(registerTargetIndex)) >> (getRegisterValue(registerSourceIndex) & 0x1f));
	return INSTRUCTION_TYPE_SRAV;
}

Cpu::InstructionType Cpu::opcodeSRLV(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	// Shift amount is truncated to 5 bits
	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(registerTargetIndex) >> (getRegisterValue(registerSourceIndex) & 0x1f));
	return INSTRUCTION_TYPE_SRLV;
}

Cpu::InstructionType Cpu::opcodeMULTU(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint64_t registerSourceValue = getRegisterValue(registerSourceIndex);
	uint64_t registerTargetValue = getRegisterValue(registerTargetIndex);

	uint64_t multResult = registerSourceValue * registerTargetValue;

	m_hi = (multResult >> 32);
	m_lo = (uint32_t)multResult;

	return INSTRUCTION_TYPE_MULTU;
}

Cpu::InstructionType Cpu::opcodeXOR(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	setRegisterValue(instruction.getRegisterDestinationIndex(), getRegisterValue(registerSourceIndex) ^ getRegisterValue(registerTargetIndex));
	return INSTRUCTION_TYPE_XOR;
}

Cpu::InstructionType Cpu::opcodeBREAK(const Instruction& instruction)
{
	exception(Exception::EXCEPTION_BREAK);
	return INSTRUCTION_TYPE_BREAK;
}

Cpu::InstructionType Cpu::opcodeMULT(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	int64_t registerSourceValue = (int32_t)getRegisterValue(registerSourceIndex);
	int64_t registerTargetValue = (int32_t)getRegisterValue(registerTargetIndex);

	uint64_t multResult = registerSourceValue * registerTargetValue;

	m_hi = (multResult >> 32);
	m_lo = (uint32_t)multResult;

	return INSTRUCTION_TYPE_MULT;
}

Cpu::InstructionType Cpu::opcodeSUB(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	int32_t registerSourceValue = getRegisterValue(registerSourceIndex);
	int32_t registerTargetValue = getRegisterValue(registerTargetIndex);

	//if ((registerSourceValue > 0 && registerTargetValue > INT_MAX + registerSourceValue) ||
	//	(registerSourceValue < 0 && registerTargetValue < INT_MIN + registerSourceValue))
	//{
	//	LOG("SUB overflow");
	//	exception(Exception::EXCEPTION_OVERFLOW);
	//	return INSTRUCTION_TYPE_OVERFLOW;
	//}
	//else
	//{
	setRegisterValue(instruction.getRegisterDestinationIndex(), registerSourceValue - registerTargetValue);
	//}

	return INSTRUCTION_TYPE_SUB;
}

Cpu::InstructionType Cpu::opcodeXORI(const Instruction& instruction)
{
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	setRegisterValue(registerTargetIndex, getRegisterValue(registerSourceIndex) ^ instruction.getImmediateValue());
	return INSTRUCTION_TYPE_XORI;
}

Cpu::InstructionType Cpu::opcodeCOP1(const Instruction& instruction)
{
	exception(Exception::EXCEPTION_COPROCESSOR_ERROR);
	return INSTRUCTION_TYPE_COP1;
}

Cpu::InstructionType Cpu::opcodeCOP2(const Instruction& instruction)
{
	InstructionType instructionType = INSTRUCTION_TYPE_UNKNOWN;

	uint32_t copOpcode = instruction.getCopOpcodeValue();

	if (copOpcode & 0x10)
	{
		// GTE command.
		m_gte.command(instruction.getInstructionOpcode());
		instructionType = INSTRUCTION_TYPE_GTE_COMMAND;
	}
	else
	{
		switch (copOpcode)
		{
		case 0b00000:
		{
			instructionType = opcodeMFC2(instruction);
			break;
		}
		case 0b00010:
		{
			instructionType = opcodeCFC2(instruction);
			break;
		}
		case 0b00100:
		{
			instructionType = opcodeMTC2(instruction);
			break;
		}
		case 0b00110:
		{
			instructionType = opcodeCTC2(instruction);
			break;
		}
		default:
		{
			assert(("Unhandled GTE instruction", false));
		}
		}
	}
	return instructionType;
}

Cpu::InstructionType Cpu::opcodeCTC2(const Instruction& instruction)
{
	RegisterIndex cpuRegisterIndex = instruction.getRegisterTargetIndex();
	uint32_t copRegister = instruction.getRegisterDestinationIndex().getRegisterIndex();

	m_gte.setControl(copRegister, getRegisterValue(cpuRegisterIndex));
	return INSTRUCTION_TYPE_CTC2;
}

Cpu::InstructionType Cpu::opcodeCOP3(const Instruction& instruction)
{
	exception(Exception::EXCEPTION_COPROCESSOR_ERROR);
	return INSTRUCTION_TYPE_COP3;
}

Cpu::InstructionType Cpu::opcodeLWL(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + signExtendedImmediateValue;

	// This instruction bypasses the load delay restriction : this instruction will merge
	// the contents with the value currently being loaded if it needs to be
	uint32_t currentRegisterValue = m_outRegs[registerTargetIndex.getRegisterIndex()];

	// Next we load the *aligned* word containing the first addressed byte
	uint32_t alignedAddr = addr & ~0x3;

	Instruction instructionLoaded = load<uint32_t>(alignedAddr);
	Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	uint32_t alignedWord = instructionLoaded.getInstructionOpcode();

	// Depending on the address alignment we fetch the 1, 2, 3 or 4 *most* significant bytes
	// and put them in the target register
	uint32_t alignedValue = 0;
	switch (addr & 0x3)
	{
	case 0:
		alignedValue = (currentRegisterValue & 0x00ffffff) | (alignedWord << 24);
		break;
	case 1:
		alignedValue = (currentRegisterValue & 0x0000ffff) | (alignedWord << 16);
		break;
	case 2:
		alignedValue = (currentRegisterValue & 0x000000ff) | (alignedWord << 8);
		break;
	case 3:
		alignedValue = alignedWord;
		break;
	}

	// Put the load in the delay slot
	m_load = RegisterData(registerTargetIndex, alignedValue);
	return INSTRUCTION_TYPE_LWL;
}

Cpu::InstructionType Cpu::opcodeLWR(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	int32_t addr = getRegisterValue(registerSourceIndex) + signExtendedImmediateValue;

	// This instruction bypasses the load delay restriction : this instruction will merge
	// the contents with the value currently being loaded if it needs to be
	uint32_t currentRegisterValue = m_outRegs[registerTargetIndex.getRegisterIndex()];

	// Next we load the *aligned* word containing the first addressed byte
	uint32_t alignedAddr = addr & ~0x3;

	Instruction instructionLoaded = load<uint32_t>(alignedAddr);
	Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	uint32_t alignedWord = instructionLoaded.getInstructionOpcode();

	// Depending on the address alignment we fetch the 1, 2, 3 or 4 *most* significant bytes
	// and put them in the target register
	uint32_t alignedValue = 0;
	switch (addr & 0x3)
	{
	case 0:
		alignedValue = alignedWord;
		break;
	case 1:
		alignedValue = (currentRegisterValue & 0xff000000) | (alignedWord >> 8);
		break;
	case 2:
		alignedValue = (currentRegisterValue & 0xffff0000) | (alignedWord >> 16);
		break;
	case 3:
		alignedValue = (currentRegisterValue & 0xffffff00) | (alignedWord >> 24);
		break;
	}

	// Put the load in the delay slot
	m_load = RegisterData(registerTargetIndex, alignedValue);
	return INSTRUCTION_TYPE_LWR;
}

Cpu::InstructionType Cpu::opcodeSWL(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + signExtendedImmediateValue;
	uint32_t registerTargetValue = getRegisterValue(registerTargetIndex);

	uint32_t alignedAddr = addr & ~0x3;

	// Load the current value for the aligned word at the target address
	Instruction instructionLoaded = load<uint32_t>(alignedAddr);
	Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	uint32_t currentMem = instructionLoaded.getInstructionOpcode();

	uint32_t mem = 0;
	switch (addr & 0x3)
	{
	case 0:
		mem = (currentMem & 0xffffff00) | (registerTargetValue >> 24);
		break;
	case 1:
		mem = (currentMem & 0xffff0000) | (registerTargetValue >> 16);
		break;
	case 2:
		mem = (currentMem & 0xff000000) | (registerTargetValue >> 8);
		break;
	case 3:
		mem = registerTargetValue;
		break;
	}

	store<uint32_t>(alignedAddr, mem);
	return INSTRUCTION_TYPE_SWL;
}

Cpu::InstructionType Cpu::opcodeSWR(const Instruction& instruction)
{
	uint32_t signExtendedImmediateValue = instruction.getSignExtendedImmediateValue();
	RegisterIndex registerSourceIndex = instruction.getRegisterSourceIndex();
	RegisterIndex registerTargetIndex = instruction.getRegisterTargetIndex();

	uint32_t addr = getRegisterValue(registerSourceIndex) + signExtendedImmediateValue;
	uint32_t registerTargetValue = getRegisterValue(registerTargetIndex);

	uint32_t alignedAddr = addr & ~0x3;

	// Load the current value for the aligned word at the target address
	Instruction instructionLoaded = load<uint32_t>(alignedAddr);
	Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
		instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
		return INSTRUCTION_TYPE_UNKNOWN;
	if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
		return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

	uint32_t currentMem = instructionLoaded.getInstructionOpcode();

	uint32_t mem = 0;
	switch (addr & 0x3)
	{
	case 0:
		mem = registerTargetValue;
		break;
	case 1:
		mem = (currentMem & 0x000000ff) | (registerTargetValue << 8);
		break;
	case 2:
		mem = (currentMem & 0x0000ffff) | (registerTargetValue << 16);
		break;
	case 3:
		mem = (currentMem & 0x00ffffff) | (registerTargetValue << 24);
		break;
	}

	store<uint32_t>(alignedAddr, mem);
	return INSTRUCTION_TYPE_SWR;
}

Cpu::InstructionType Cpu::opcodeLWC0(const Instruction& instruction)
{
	// Not supported by this coprocessor
	exception(Exception::EXCEPTION_COPROCESSOR_ERROR);
	return INSTRUCTION_TYPE_LWC0;
}

Cpu::InstructionType Cpu::opcodeLWC1(const Instruction& instruction)
{
	// Not supported by this coprocessor
	exception(Exception::EXCEPTION_COPROCESSOR_ERROR);
	return INSTRUCTION_TYPE_LWC1;
}

Cpu::InstructionType Cpu::opcodeLWC2(const Instruction& instruction)
{
	uint32_t addr = getRegisterValue(instruction.getRegisterSourceIndex()) + instruction.getSignExtendedImmediateValue();

	// Address must be 32 bit aligned.
	if ((addr % 4) == 0)
	{
		Instruction instructionLoaded = load<uint32_t>(addr);
		Instruction::InstructionStatus instructionStatus = instructionLoaded.getInstructionStatus();
		if (instructionStatus == Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS ||
			instructionStatus == Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH)
			return INSTRUCTION_TYPE_UNKNOWN;
		if (instructionStatus == Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED)
			return INSTRUCTION_TYPE_NOT_IMPLEMENTED;

		// Send to coprocessor.
		m_gte.setData(instruction.getRegisterTargetIndex().getRegisterIndex(), instructionLoaded.getInstructionOpcode());
	}
	else
	{
		exception(Exception::EXCEPTION_LOAD_ADDRESS_ERROR);
	}

	return INSTRUCTION_TYPE_LWC2;
}

Cpu::InstructionType Cpu::opcodeLWC3(const Instruction& instruction)
{
	// Not supported by this coprocessor
	exception(Exception::EXCEPTION_COPROCESSOR_ERROR);
	return INSTRUCTION_TYPE_LWC3;
}

Cpu::InstructionType Cpu::opcodeSWC0(const Instruction& instruction)
{
	// Not supported by this coprocessor
	exception(Exception::EXCEPTION_COPROCESSOR_ERROR);
	return INSTRUCTION_TYPE_SWC0;
}

Cpu::InstructionType Cpu::opcodeSWC1(const Instruction& instruction)
{
	// Not supported by this coprocessor
	exception(Exception::EXCEPTION_COPROCESSOR_ERROR);
	return INSTRUCTION_TYPE_SWC1;
}

Cpu::InstructionType Cpu::opcodeSWC2(const Instruction& instruction)
{
	uint32_t addr = getRegisterValue(instruction.getRegisterSourceIndex()) + instruction.getSignExtendedImmediateValue();
	uint32_t data = m_gte.getData(instruction.getRegisterTargetIndex().getRegisterIndex());

	// Address must be 32 bit aligned.
	if ((addr % 4) == 0)
		store<uint32_t>(addr, data);
	else
		exception(Exception::EXCEPTION_LOAD_ADDRESS_ERROR);

	return INSTRUCTION_TYPE_SWC2;
}

Cpu::InstructionType Cpu::opcodeSWC3(const Instruction& instruction)
{
	// Not supported by this coprocessor
	exception(Exception::EXCEPTION_COPROCESSOR_ERROR);
	return INSTRUCTION_TYPE_SWC3;
}

Cpu::InstructionType Cpu::opcodeMFC2(const Instruction& instruction)
{
	m_load = RegisterData(instruction.getRegisterTargetIndex(), m_gte.getData(instruction.getRegisterDestinationIndex().getRegisterIndex()));
	return INSTRUCTION_TYPE_MFC2;
}

Cpu::InstructionType Cpu::opcodeCFC2(const Instruction& instruction)
{
	m_load = RegisterData(instruction.getRegisterTargetIndex(), m_gte.getControl(instruction.getRegisterDestinationIndex().getRegisterIndex()));
	return INSTRUCTION_TYPE_CFC2;
}

Cpu::InstructionType Cpu::opcodeMTC2(const Instruction& instruction)
{
	m_gte.setData(instruction.getRegisterDestinationIndex().getRegisterIndex(), getRegisterValue(instruction.getRegisterTargetIndex()));
	return INSTRUCTION_TYPE_MTC2;
}

Cpu::InstructionType Cpu::opcodeIllegal(const Instruction& instruction)
{
	LOG("Illegal instruction 0x" << std::hex << instruction.getInstructionOpcode());
	exception(Exception::EXCEPTION_UNKNOWN_INSTRUCTION);
	return INSTRUCTION_TYPE_UNKNOWN;
}

uint32_t Cpu::getRegisterValue(RegisterIndex registerIndex) const
{
	assert(("Index out of boundaries", registerIndex.getRegisterIndex() < _countof(m_regs)));
	return m_regs[registerIndex.getRegisterIndex()];
}

void Cpu::setRegisterValue(RegisterIndex registerIndex, uint32_t value)
{
	assert(("Index out of boundaries", registerIndex.getRegisterIndex() < _countof(m_outRegs)));
	if (registerIndex.getRegisterIndex() > 0) m_outRegs[registerIndex.getRegisterIndex()] = value;
}