#include "pscx_memory.h"

using namespace pscx_memory;

uint32_t pscx_memory::maskRegion(uint32_t addr)
{
	// Index address space in 512MB chunks
	size_t index = (addr >> 29);
	return addr & REGION_MASK[index];
}

// ***************** CacheControl implementation ******************
bool CacheControl::icacheEnabled() const
{
	return m_cache & 0x800;
}

bool CacheControl::tagTestMode() const
{
	return m_cache & 4;
}
