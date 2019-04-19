#pragma once

#include <cstdint>

// GTE division algorithm: returns a saturated 1.16 value. Divisions by 0
// shouldn't occur since we clip against the near plane but they would saturate to 0x1ffff anyway.
// The algorithm is based on Newton-Raphson.
uint32_t divide(uint16_t numerator, uint16_t divisor);
uint32_t calculateReciprocal(uint16_t divisor);
