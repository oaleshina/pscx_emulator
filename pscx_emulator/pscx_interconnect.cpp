#include "pscx_interconnect.h"

#include <iostream>

// Mask a CPU address to remove the region bits
static uint32_t maskRegion(uint32_t addr)
{
	// Index address space in 512MB chunks
	size_t index = (addr >> 29);

	return addr & REGION_MASK[index];
}

Interconnect::Interconnect(Bios bios) :
	m_bios(bios),
	m_range(0x1f801000, 36) // Memory latency and expansion mapping
{
}

uint32_t Interconnect::load32(uint32_t addr)
{
	uint32_t offset = 0;
	if ((addr & 0x11) != 0)
	{
		std::cout << "unaligned load32 address 0x" << std::hex << addr << std::endl;
		return offset;
	}

	if (m_bios.m_range.contains(addr, offset))
		return m_bios.load32(offset);

	std::cout << "unhandled fetch32 at address 0x" << std::hex << addr << std::endl;
	return offset;
}

void Interconnect::store32(uint32_t addr, uint32_t value)
{
	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (m_range.contains(targetPeripheralAddress, offset))
	{
		switch (offset)
		{
		case 0:
		{
			// Expansion 1 base address
			if (value != 0x1f000000)
				std::cout << "bad expansion 1 base address 0x" << std::hex << value << std::endl;
			break;
		}
		case 4:
		{
			// Expansion 2 base address
			if (value != 0x1f802000)
				std::cout << "bad expansion 2 base address 0x" << std::hex << value << std::endl;
			break;
		}
		default:	
			// Warn
			std::cout << "unhandled write to MEMCONTROL register" << std::endl;	
		}
		return;
	}

	std::cout << "unhandled store32 into address 0x" << std::hex << addr << std::endl;
}
