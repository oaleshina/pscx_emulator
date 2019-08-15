#pragma once

#include "pscx_memory.h"
#include <iostream>

#include <vector>

using namespace pscx_memory;

// Main PlayStation RAM: 2 Megabytes
const uint32_t MAIN_RAM_SIZE = 2 * 1024 * 1024;

// ScratchPad (data cache used as fast RAM): 1 Kilobyte
const uint32_t SCRATCH_PAD_SIZE = 1024;

struct ScratchPad
{
	ScratchPad()
	{
		// Instantiate scratchpad with garbage values
		memset(m_data, 0xdb, SCRATCH_PAD_SIZE);
	}

	template<typename T>
	T load(uint32_t offset) const;

	template<typename T>
	void store(uint32_t offset, T value);

private:
	uint8_t m_data[SCRATCH_PAD_SIZE];
};

struct Ram
{
	Ram()
	{
		std::cout << "RAM initialization\n";
		m_data.resize(MAIN_RAM_SIZE);

		// Default RAM contants are garbage
		memset(m_data.data(), 0xca, MAIN_RAM_SIZE);
	}

	// RAM buffer
	std::vector<uint8_t> m_data;

	template<typename T>
	T load(uint32_t offset) const;

	//uint32_t load32(uint32_t offset) const; // Fetch the 32 bit little endian word at 'offset'
	//uint16_t load16(uint32_t offset) const; // Fetch the 16 bit little endian halfword at 'offset'
	//uint8_t  load8 (uint32_t offset) const; // Fetch the byte at 'offset'

	template<typename T>
	void store(uint32_t offset, T value);

	//void store32(uint32_t offset, uint32_t value); // Store the 32 bit little endian word 'value' into 'offset'
	//void store16(uint32_t offset, uint16_t value); // Store the 16 bit little endian halfword 'value' into 'offset'
	//void store8 (uint32_t offset, uint8_t value);  // Store the byte 'value' into 'offset'
};
