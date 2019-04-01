#pragma once

#include <cstdint>
#include <vector>

// Compute the CRC32 of data
// Uses polynomial (x^16 + x^15 + x^2 + 1) * (x^16 + x^2 + x + 1)
uint32_t crc32(const std::vector<uint8_t>& data);
