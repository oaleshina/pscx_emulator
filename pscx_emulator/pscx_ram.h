#pragma once

#include "pscx_memory.h"

#include <vector>

using namespace pscx_memory;

struct Ram
{
	Ram()
	{
		m_data.resize(2 * 1024 * 1024); // 2 MB

		// Default RAM contants are garbage
		memset(m_data.data(), 0xca, m_data.size());
	}

	// RAM buffer
	std::vector<uint8_t> m_data;

	// Fetch the 32 bit little endian word at 'offset'
	uint32_t load32(uint32_t offset) const;

	// Store the 32 bit little endian word 'value' into 'offset'
	void store32(uint32_t offset, uint32_t value);
};
