#pragma once

#include "pscx_memory.h"

#include <vector>
#include <string>

using namespace pscx_memory;

// BIOS image
struct Bios
{
	enum class BiosState
	{
		BIOS_STATE_SUCCESS,
		BIOS_STATE_INCORRECT_FILENAME,
		BIOS_STATE_INVALID_BIOS_SIZE,
		BIOS_STATE_COUNT
	};

	// BIOS memory
	std::vector<uint8_t> m_data;

	// Load a BIOS image from the file that is located in 'path'
	BiosState loadBios(const std::string& path);

	template<typename T>
	T load(uint32_t offset) const;
};