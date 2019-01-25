#include "pscx_ram.h"

template<> uint32_t Ram::load<uint32_t>(uint32_t offset) const
{
	uint32_t b0 = m_data[offset + 0];
	uint32_t b1 = m_data[offset + 1];
	uint32_t b2 = m_data[offset + 2];
	uint32_t b3 = m_data[offset + 3];

	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

template<> uint16_t Ram::load<uint16_t>(uint32_t offset) const
{
	uint16_t b0 = m_data[offset];
	uint16_t b1 = m_data[offset + 1];

	return b0 | (b1 << 8);
}

template<> uint8_t Ram::load<uint8_t>(uint32_t offset) const
{
	return m_data[offset];
}

template<> void Ram::store<uint32_t>(uint32_t offset, uint32_t value)
{
	m_data[offset + 0] = (uint8_t) value;
	m_data[offset + 1] = (uint8_t)(value >> 8);
	m_data[offset + 2] = (uint8_t)(value >> 16);
	m_data[offset + 3] = (uint8_t)(value >> 24);
}

template<> void Ram::store<uint16_t>(uint32_t offset, uint16_t value)
{
	m_data[offset] = (uint8_t)value;
	m_data[offset + 1] = (uint8_t)(value >> 8);
}

template<> void Ram::store<uint8_t>(uint32_t offset, uint8_t value)
{
	m_data[offset] = value;
}
