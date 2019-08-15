#include "pscx_ram.h"

template<> uint32_t Ram::load<uint32_t>(uint32_t offset) const
{
	// The two MSB are ignored, the 2 MB RAM is mirrored four times
	// over the first 8 MB of address space.
	uint32_t byteOffset = offset & 0x1fffff;

	uint32_t b0 = m_data[byteOffset + 0];
	uint32_t b1 = m_data[byteOffset + 1];
	uint32_t b2 = m_data[byteOffset + 2];
	uint32_t b3 = m_data[byteOffset + 3];

	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

template<> uint16_t Ram::load<uint16_t>(uint32_t offset) const
{
	uint32_t byteOffset = offset & 0x1fffff;

	uint16_t b0 = m_data[byteOffset];
	uint16_t b1 = m_data[byteOffset + 1];

	return b0 | (b1 << 8);
}

template<> uint8_t Ram::load<uint8_t>(uint32_t offset) const
{
	return m_data[offset & 0x1fffff];
}

template<> void Ram::store<uint32_t>(uint32_t offset, uint32_t value)
{
	// The two MSB are ignored, the 2 MB RAM is mirrored four times
	// over the first 8 MB of address space.
	uint32_t byteOffset = offset & 0x1fffff;

	m_data[byteOffset + 0] = (uint8_t) value;
	m_data[byteOffset + 1] = (uint8_t)(value >> 8);
	m_data[byteOffset + 2] = (uint8_t)(value >> 16);
	m_data[byteOffset + 3] = (uint8_t)(value >> 24);
}

template<> void Ram::store<uint16_t>(uint32_t offset, uint16_t value)
{
	uint32_t byteOffset = offset & 0x1fffff;

	m_data[byteOffset] = (uint8_t)value;
	m_data[byteOffset + 1] = (uint8_t)(value >> 8);
}

template<> void Ram::store<uint8_t>(uint32_t offset, uint8_t value)
{
	m_data[offset & 0x1fffff] = value;
}

// ********************** ScratchPad implementation **********************
template<> uint32_t ScratchPad::load<uint32_t>(uint32_t offset) const
{
	uint32_t value(0x0);

	for (size_t i = 0; i < sizeof(uint32_t); ++i)
		value |= m_data[offset + i] << (i * 8);

	return value;
}

template<> uint16_t ScratchPad::load<uint16_t>(uint32_t offset) const
{
	uint32_t value(0x0);

	for (size_t i = 0; i < sizeof(uint16_t); ++i)
		value |= m_data[offset + i] << (i * 8);

	return value;
}

template<> uint8_t ScratchPad::load<uint8_t>(uint32_t offset) const
{
	uint32_t value(0x0);

	for (size_t i = 0; i < sizeof(uint8_t); ++i)
		value |= m_data[offset + i] << (i * 8);

	return value;
}

template<> void ScratchPad::store<uint32_t>(uint32_t offset, uint32_t value)
{
	for (size_t i = 0; i < sizeof(uint32_t); ++i)
		m_data[offset + i] = value >> (i * 8);
}

template<> void ScratchPad::store<uint16_t>(uint32_t offset, uint16_t value)
{
	for (size_t i = 0; i < sizeof(uint16_t); ++i)
		m_data[offset + i] = value >> (i * 8);
}

template<> void ScratchPad::store<uint8_t>(uint32_t offset, uint8_t value)
{
	for (size_t i = 0; i < sizeof(uint8_t); ++i)
		m_data[offset + i] = value >> (i * 8);
}
