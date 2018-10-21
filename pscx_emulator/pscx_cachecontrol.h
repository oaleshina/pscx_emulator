#pragma once

#include "pscx_memory.h"

using namespace pscx_memory;

struct CacheControl
{
	CacheControl(uint32_t cacheControlRegister) :
		m_cacheControlRegister(cacheControlRegister)
	{}

	uint32_t m_cacheControlRegister;
};

