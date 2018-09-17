#pragma once

#include "pscx_bios.h"

// Mask array used to strip the region bits of the address. The
// mask is selected using the 3 MSBs of the address so each entry
// effectively matches 512 kB of the address space. KSEG2 is not
// touched since it doesn't share anything with the other regions.
const uint32_t REGION_MASK[8] =
{
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, // KUSEG: 2048MB
	0x7fffffff,                                     // KSEG0:  512MB
	0x1fffffff,                                     // KSEG1:  512MB
	0xffffffff, 0xffffffff                          // KSEG2: 1024MB
};

// Global interconnect
struct Interconnect
{
	Interconnect(Bios bios);

	// Load 32 bit word at 'addr'
	uint32_t load32(uint32_t addr);

	// Store 32 bit word 'val' into 'addr'
	void store32(uint32_t addr, uint32_t value);

	// Basic Input/Output memory
	Bios m_bios;
	Range m_range;
};