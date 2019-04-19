#pragma once

#include <cstdint>

// CD minute:second:frame timestamp given as 3 pairs of BSD encoded bytes (4 bits per digit).
struct MinuteSecondFrame
{
	MinuteSecondFrame(uint8_t minute, uint8_t second, uint8_t frame);

	uint8_t getMinute() const;
	uint8_t getSecond() const;
	uint8_t getFrame() const;

	bool operator!=(const MinuteSecondFrame& minuteSecondFrame);
	bool operator>=(const MinuteSecondFrame& minuteSecondFrame);

	// Create a 00:00:00 MSF timestamp
	static MinuteSecondFrame createZeroTimestamp();
	static MinuteSecondFrame fromBCD(uint8_t minute, uint8_t second, uint8_t frame);

	// Convert an MSF coordinate into a sector index. In this convention
	// sector 0 is 00:00:00 ( i.e. before track 01's pregap ).
	uint32_t getSectorIndex() const;

	// Return the MSF timestamp of the next sector
	MinuteSecondFrame getNextSector() const;

	// Packing the MSF in a single u32 makes it easier to do comparisons.
	uint32_t packToU32BCD() const;

private:
	uint8_t m_minute;
	uint8_t m_second;
	uint8_t m_frame;
};
