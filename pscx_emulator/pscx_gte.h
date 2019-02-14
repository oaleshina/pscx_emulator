#pragma once

#include <cstdint>

// Geometry transform engine

struct Gte
{
	Gte() :
		m_redBackgroundColor(0x0),
		m_greenBackgroundColor(0x0),
		m_blueBackgroundColor(0x0),
		m_redFarComponent(0x0),
		m_greenFarComponent(0x0),
		m_blueFarComponent(0x0),
		m_offsetX(0x0),
		m_offsetY(0x0),
		m_projectionPlaneDistance(0x0),
		m_depthQueingCoefficient(0x0),
		m_depthQueingOffset(0x0),
		m_zScaleFactor3(0x0),
		m_zScaleFactor4(0x0)
	{}

	// Store value in one of the "control" registers. Used by the CTC2 opcode.
	void setControl(uint32_t reg, uint32_t value);

private:
	// Background color red component: signed 20.12
	int32_t m_redBackgroundColor;
	// Background color green component: signed 20.12
	int32_t m_greenBackgroundColor;
	// Background color blue component: signed 20.12
	int32_t m_blueBackgroundColor;

	// Far color red component: signed 28.4
	int32_t m_redFarComponent;
	// Far color green component: signed 28.4
	int32_t m_greenFarComponent;
	// Far color blue component: signed 28.4
	int32_t m_blueFarComponent;

	// Screen offset X: signed 16.16
	int32_t m_offsetX;
	// Screen offset Y: signed 16.16
	int32_t m_offsetY;

	// Projection plane distance
	uint16_t m_projectionPlaneDistance;
	
	// Depth queing coefficient: signed 8.8
	int16_t m_depthQueingCoefficient;
	// Depth queing offset: signed 8.24
	int32_t m_depthQueingOffset;

	// Scale factor when computing the average of 3 Z values (triangle): signed 4.12
	int16_t m_zScaleFactor3;
	// Scale factor when computing the average of 4 Z values (quad): signed 4.12
	int16_t m_zScaleFactor4;
};
