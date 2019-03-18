#pragma once

#include <cstdint>

namespace pscx_memory
{
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

	// Mask a CPU address to remove the region bits
	uint32_t maskRegion(uint32_t addr);

	struct Range
	{
		Range(uint32_t start, uint32_t length) :
			m_start(start),
			m_length(length)
		{}

		// Return 'offset' if addr is contained in the interval
		bool contains(uint32_t addr, uint32_t &offset) const
		{
			if (addr >= m_start && addr < m_start + m_length)
			{
				offset = addr - m_start;
				return true;
			}
			return false;
		}

		uint32_t m_start, m_length;
	};

	struct RegisterIndex
	{
		RegisterIndex(uint32_t index) : m_index(index) {};
		uint32_t getRegisterIndex() const { return m_index; }

	private:
		uint32_t m_index;
	};

	struct CacheControl
	{
		CacheControl(uint32_t cache) : m_cache(cache) {}

		// Return true if the instruction cache is enabled
		bool icacheEnabled() const;
		bool tagTestMode() const;

	private:
		uint32_t m_cache;
	};

	// Address map declarations
	const Range RAM          = Range(0x00000000, 2 * 1024 * 1024);
	const Range BIOS         = Range(0x1fc00000, 512 * 1024);

	// Unknown registers. The name comes from mednafen
	const Range MEM_CONTROL  = Range(0x1f801000, 36);

	// Register that has something to do with RAM configuration,
	// configured by the BIOS
	const Range RAM_SIZE     = Range(0x1f801060, 4);
	
	// Cache control register. Full address since it's in KSEG2
	const Range CACHE_CONTROL = Range(0xfffe0130, 4);

	// SPU registers
	const Range SPU           = Range(0x1f801c00, 640);

	// Expansion region 1
	const Range EXPANSION_1   = Range(0x1f000000, 512 * 1024);

	// Expansion region 2
	const Range EXPANSION_2   = Range(0x1f802000, 66);

	// Interrupt Control registers ( status and mask )
	const Range IRQ_CONTROL   = Range(0x1f801070, 8);

	// Timer registers
	const Range TIMERS        = Range(0x1f801100, 0x30);

	// Direct Memory Access registers
	const Range DMA           = Range(0x1f801080, 0x80);

	// GPU registers
	const Range GPU           = Range(0x1f801810, 8);

	// CDROM controller
	const Range CDROM         = Range(0x1f801800, 0x4);

	// Gamepad and memory card controller
	const Range PAD_MEMCARD   = Range(0x1f801040, 32);
}
