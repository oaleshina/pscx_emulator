#include <cstdlib>
#include <cassert>

#include "pscx_minutesecondframe.h"

// ********************** MinuteSecondFrame implementation **********************
MinuteSecondFrame::MinuteSecondFrame(uint8_t minute, uint8_t second, uint8_t frame) :
	m_minute(minute),
	m_second(second),
	m_frame(frame)
{
}

uint8_t MinuteSecondFrame::getMinute() const
{
	return m_minute;
}

uint8_t MinuteSecondFrame::getSecond() const
{
	return m_second;
}

uint8_t MinuteSecondFrame::getFrame() const
{
	return m_frame;
}

bool MinuteSecondFrame::operator!=(const MinuteSecondFrame& minuteSecondFrame)
{
	uint32_t byteMinuteSecondFrameLeft = this->packToU32BCD();
	uint32_t byteMinuteSecondFrameRight = minuteSecondFrame.packToU32BCD();
	return byteMinuteSecondFrameLeft != byteMinuteSecondFrameRight;
}

bool MinuteSecondFrame::operator> (const MinuteSecondFrame& minuteSecondFrame)
{
	uint32_t byteMinuteSecondFrameLeft = this->packToU32BCD();
	uint32_t byteMinuteSecondFrameRight = minuteSecondFrame.packToU32BCD();
	return byteMinuteSecondFrameLeft > byteMinuteSecondFrameRight;
}

MinuteSecondFrame MinuteSecondFrame::createZeroTimestamp()
{
	return MinuteSecondFrame(0x0, 0x0, 0x0);
}

MinuteSecondFrame MinuteSecondFrame::fromBCD(uint8_t minute, uint8_t second, uint8_t frame)
{
	// Make sure that we have valid BCD data
	uint8_t bcd[] = { minute, second, frame };
	for (size_t i = 0; i < _countof(bcd); ++i)
	{
		assert(!(bcd[i] > 0x99 || (bcd[i] & 0xf) > 0x9), "Invalid MSF");
	}

	// Make sure the frame and seconds make sense.
	// There are only 75 frames per second and 60 seconds per minute.
	assert(!(second >= 0x60 || frame >= 0x75), "Invalid MSF");

	return MinuteSecondFrame(minute, second, frame);
}

uint32_t MinuteSecondFrame::getSectorIndex() const
{
	auto fromBCD = [](uint8_t bcd) -> uint8_t { return (bcd >> 4) * 10 + (bcd & 0xf); };

	uint32_t minute = fromBCD(m_minute);
	uint32_t second = fromBCD(m_second);
	uint32_t frame = fromBCD(m_frame);

	// 60 seconds in a minute, 75 sectors (frames) in a second
	return 60 * 75 * minute + 75 * second + frame;
}

MinuteSecondFrame MinuteSecondFrame::getNextSector() const
{
	auto bcdInc = [](uint8_t bcd) -> uint8_t { return (bcd & 0xf) < 9 ? bcd + 1 : (bcd & 0xf0) + 0x10; };

	if (m_frame < 0x74)
		return MinuteSecondFrame(m_minute, m_second, bcdInc(m_frame));

	if (m_second < 0x59)
		return MinuteSecondFrame(m_minute, bcdInc(m_second), m_frame);

	if (m_minute < 0x99)
		return MinuteSecondFrame(bcdInc(m_minute), 0x0, 0x0);

	assert(0, "MSF overflow");
}

uint32_t MinuteSecondFrame::packToU32BCD() const
{
	return (((uint32_t)m_minute) << 16) | (((uint32_t)m_second) << 8) | ((uint32_t)m_frame);
}
