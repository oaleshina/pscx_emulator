#include "pscx_memory.h"

using namespace pscx_memory;

uint32_t pscx_memory::maskRegion(uint32_t addr)
{
	// Index address space in 512MB chunks
	size_t index = (addr >> 29);
	return addr & REGION_MASK[index];
}

// ***************** StatusRegister implementation ******************
uint32_t StatusRegister::getStatusRegister() const
{
	return m_status;
}

bool StatusRegister::isCacheIsolated() const
{
	return m_status & 0x10000;
}

uint32_t StatusRegister::exceptionHandler() const
{
	if (m_status & (1 << 22))
		return 0xbfc00180;
	return 0x80000080;
}

void StatusRegister::enterException()
{
	uint32_t mode = m_status & 0x3f;
	m_status &= (~0x3f);
	m_status |= (mode << 2) & 0x3f;
}

void StatusRegister::returnFromException()
{
	uint32_t mode = m_status & 0x3f;
	m_status &= (~0x3f);
	m_status |= (mode >> 2);
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
