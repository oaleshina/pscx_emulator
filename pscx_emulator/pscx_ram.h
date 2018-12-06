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

	uint32_t load32(uint32_t offset) const; // Fetch the 32 bit little endian word at 'offset'
	uint16_t load16(uint32_t offset) const; // Fetch the 16 bit little endian halfword at 'offset'
	uint8_t  load8 (uint32_t offset) const; // Fetch the byte at 'offset'

	void store32(uint32_t offset, uint32_t value); // Store the 32 bit little endian word 'value' into 'offset'
	void store16(uint32_t offset, uint16_t value); // Store the 16 bit little endian halfword 'value' into 'offset'
	void store8 (uint32_t offset, uint8_t value);  // Store the byte 'value' into 'offset'
};
