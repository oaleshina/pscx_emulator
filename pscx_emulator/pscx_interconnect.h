#pragma once

#include "pscx_common.h"
#include "pscx_bios.h"
#include "pscx_ram.h"
#include "pscx_memory.h"
#include "pscx_instruction.h"

using namespace pscx_memory;

// Global interconnect
struct Interconnect
{
	Interconnect(Bios bios);

	// Load 32 bit word at 'addr'
	Instruction load32(uint32_t addr) const;

	// Load 16 bit halfword at 'addr'
	Instruction load16(uint32_t addr) const;

	// Load byte at 'addr'
	Instruction load8(uint32_t addr) const;

	void store32(uint32_t addr, uint32_t value); // Store 32 bit word 'val' into 'addr'
	void store16(uint32_t addr, uint16_t value); // Store 16 bit word 'val' into 'addr'
	void store8 (uint32_t addr, uint8_t  value); // Store 8 bit word 'val' into 'addr'

	// Basic Input/Output memory
	Bios m_bios;
	Ram m_ram;
};