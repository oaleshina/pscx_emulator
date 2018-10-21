#include "pscx_memory.h"

uint32_t pscx_memory::maskRegion(uint32_t addr)
{
	// Index address space in 512MB chunks
	size_t index = (addr >> 29);

	return addr & REGION_MASK[index];
}