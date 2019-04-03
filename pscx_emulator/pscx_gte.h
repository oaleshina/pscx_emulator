#pragma once

#include <cstdint>
#include <cstring>

// Geometry transform engine.

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
		m_zScaleFactor4(0x0),
		// Vectors initialization.
		m_translationVector{0x0, 0x0, 0x0},
		m_vector0{0x0, 0x0, 0x0},
		m_vector1{0x0, 0x0, 0x0},
		m_vector2{0x0, 0x0, 0x0}
	{
		// Matrices initialization.
		memset(m_lightSourceMatrix, 0x0, sizeof(m_lightSourceMatrix));
		memset(m_lightColorMatrix, 0x0, sizeof(m_lightColorMatrix));
		memset(m_rotationMatrix, 0x0, sizeof(m_rotationMatrix));
	}

	// Store value in one of the "control" registers. Used by the CTC2 opcode.
	void setControl(uint32_t reg, uint32_t value);
	// Store value in one of the "data" registers. Used by the MTC2 and LWC2 opcodes.
	void setData(uint32_t reg, uint32_t value);

	// Execute GTE command.
	void command(uint32_t command);

private:
	// Control registers.
	// Background color red component: signed 20.12.
	int32_t m_redBackgroundColor;
	// Background color green component: signed 20.12.
	int32_t m_greenBackgroundColor;
	// Background color blue component: signed 20.12.
	int32_t m_blueBackgroundColor;

	// Far color red component: signed 28.4.
	int32_t m_redFarComponent;
	// Far color green component: signed 28.4.
	int32_t m_greenFarComponent;
	// Far color blue component: signed 28.4.
	int32_t m_blueFarComponent;

	// Screen offset X: signed 16.16.
	int32_t m_offsetX;
	// Screen offset Y: signed 16.16.
	int32_t m_offsetY;

	// Projection plane distance.
	uint16_t m_projectionPlaneDistance;
	
	// Depth queing coefficient: signed 8.8.
	int16_t m_depthQueingCoefficient;
	// Depth queing offset: signed 8.24.
	int32_t m_depthQueingOffset;

	// Scale factor when computing the average of 3 Z values (triangle): signed 4.12.
	int16_t m_zScaleFactor3;
	// Scale factor when computing the average of 4 Z values (quad): signed 4.12.
	int16_t m_zScaleFactor4;

	// Translation vector: 3x signed word.
	int32_t m_translationVector[3];
	// Light source matrix: 3x3 signed 4.12.
	int16_t m_lightSourceMatrix[3][3];
	// Light color matrix: 3x3 signed 4.12.
	int16_t m_lightColorMatrix[3][3];
	// Rotation matrix: 3x3 signed 4.12.
	int16_t m_rotationMatrix[3][3];

	// Data registers.
	// Vector 0: 3x signed 4.12.
	int16_t m_vector0[3];
	// Vector 1: 3x signed 4.12.
	int16_t m_vector1[3];
	// Vector 2: 3x signed 4.12.
	int16_t m_vector2[3];
};
