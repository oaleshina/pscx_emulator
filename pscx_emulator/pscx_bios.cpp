#include "pscx_bios.h"

#include <fstream>
#include <iterator>

Bios::BiosState Bios::loadBios(const std::string& path)
{
	std::basic_ifstream<uint8_t> biosFile(path, std::ios::in | std::ios::binary);

	if (!biosFile.good())
		return BiosState::BIOS_STATE_INCORRECT_FILENAME;

	const uint32_t biosSize = 512 * 1024; // 512 kb

	// Load the BIOS
	m_data.insert(m_data.begin(), std::istreambuf_iterator<uint8_t>(biosFile), std::istreambuf_iterator<uint8_t>());

	biosFile.close();

	if (m_data.size() != biosSize)
		return BiosState::BIOS_STATE_INVALID_BIOS_SIZE;

	return BiosState::BIOS_STATE_SUCCESS;
}

template<> uint32_t Bios::load<uint32_t>(uint32_t offset) const
{
	uint32_t b0 = m_data[offset + 0];
	uint32_t b1 = m_data[offset + 1];
	uint32_t b2 = m_data[offset + 2];
	uint32_t b3 = m_data[offset + 3];

	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

template<> uint16_t Bios::load<uint16_t>(uint32_t offset) const
{
	return 0;
}

template<> uint8_t Bios::load<uint8_t>(uint32_t offset) const
{
	return m_data[offset];
}