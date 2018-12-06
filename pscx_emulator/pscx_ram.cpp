#include "pscx_ram.h"

uint32_t Ram::load32(uint32_t offset) const
{
	uint32_t b0 = m_data[offset + 0];
	uint32_t b1 = m_data[offset + 1];
	uint32_t b2 = m_data[offset + 2];
	uint32_t b3 = m_data[offset + 3];

	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

uint16_t Ram::load16(uint32_t offset) const
{
	uint16_t b0 = m_data[offset];
	uint16_t b1 = m_data[offset + 1];

	return b0 | (b1 << 8);
}

uint8_t Ram::load8(uint32_t offset) const
{
	return m_data[offset];
}

//--------------------------------------------------------------
// TODO : to implement the store32 function.
// It should store a value into the RAM memory.
//
// Rust:
// pub fn store32(&mut self, offset: u32, val: u32) {
//     let offset = offset as usize;
//
//     let b0 = val as u8;
//     let b1 = (val >> 8)  as u8;
//     let b2 = (val >> 16) as u8;
//     let b3 = (val >> 24) as u8;
//
//     self.data[offset + 0] = b0;
//     self.data[offset + 1] = b1;
//     self.data[offset + 2] = b2;
//     self.data[offset + 3] = b3;
// }
//--------------------------------------------------------------
void Ram::store32(uint32_t offset, uint32_t value)
{
	// Fixme
}

void Ram::store16(uint32_t offset, uint16_t value)
{
	m_data[offset] = (uint8_t)value;
	m_data[offset + 1] = (uint8_t)(value >> 8);
}

//--------------------------------------------------------------
// TODO : to implement the store8 function.
// It should store a value into the RAM memory.
//
// Rust:
// pub fn store8(&mut self, offset: u32, val: u8) {
//     self.data[offset as usize] = val;
// }
//--------------------------------------------------------------
void Ram::store8(uint32_t offset, uint8_t value)
{
	// Fixme
}
