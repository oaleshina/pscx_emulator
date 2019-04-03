#include <cassert>

#include "pscx_gte.h"
#include "pscx_common.h"

void Gte::setControl(uint32_t reg, uint32_t value)
{
	LOG("Set GTE control reg = 0x" << std::hex << reg << " value = 0x" << value);

	switch (reg)
	{
	case 0:
		m_rotationMatrix[0][0] = (int16_t)value;
		m_rotationMatrix[0][1] = (int16_t)(value >> 16);
		break;
	case 1:
		m_rotationMatrix[0][2] = (int16_t)value;
		m_rotationMatrix[1][0] = (int16_t)(value >> 16);
		break;
	case 2:
		m_rotationMatrix[1][1] = (int16_t)value;
		m_rotationMatrix[1][2] = (int16_t)(value >> 16);
		break;
	case 3:
		m_rotationMatrix[2][0] = (int16_t)value;
		m_rotationMatrix[2][1] = (int16_t)(value >> 16);
		break;
	case 4:
		m_rotationMatrix[2][2] = (int16_t)value;
		break;
	case 5:
		m_translationVector[0] = (int32_t)value;
		break;
	case 6:
		m_translationVector[1] = (int32_t)value;
		break;
	case 7:
		m_translationVector[2] = (int32_t)value;
		break;
	case 8:
		m_lightSourceMatrix[0][0] = (int16_t)value;
		m_lightSourceMatrix[0][1] = (int16_t)(value >> 16);
		break;
	case 9:
		m_lightSourceMatrix[0][2] = (int16_t)value;
		m_lightSourceMatrix[1][0] = (int16_t)(value >> 16);
		break;
	case 10:
		m_lightSourceMatrix[1][1] = (int16_t)value;
		m_lightSourceMatrix[1][2] = (int16_t)(value >> 16);
		break;
	case 11:
		m_lightSourceMatrix[2][0] = (int16_t)value;
		m_lightSourceMatrix[2][1] = (int16_t)(value >> 16);
		break;
	case 12:
		m_lightSourceMatrix[2][2] = (int16_t)value;
		break;
	case 13:
		m_redBackgroundColor = (int32_t)value;
		break;
	case 14:
		m_greenBackgroundColor = (int32_t)value;
		break;
	case 15:
		m_blueBackgroundColor = (int32_t)value;
		break;
	case 16:
		m_lightColorMatrix[0][0] = (int16_t)value;
		m_lightColorMatrix[0][1] = (int16_t)(value >> 16);
		break;
	case 17:
		m_lightColorMatrix[0][2] = (int16_t)value;
		m_lightColorMatrix[1][0] = (int16_t)(value >> 16);
		break;
	case 18:
		m_lightColorMatrix[1][1] = (int16_t)value;
		m_lightColorMatrix[1][2] = (int16_t)(value >> 16);
		break;
	case 19:
		m_lightColorMatrix[2][0] = (int16_t)value;
		m_lightColorMatrix[2][1] = (int16_t)(value >> 16);
		break;
	case 20:
		m_lightColorMatrix[2][2] = (int16_t)value;
		break;
	case 21:
		m_redFarComponent = (int32_t)value;
		break;
	case 22:
		m_greenFarComponent = (int32_t)value;
		break;
	case 23:
		m_blueFarComponent = (int32_t)value;
		break;
	case 24:
		m_offsetX = (int32_t)value;
		break;
	case 25:
		m_offsetY = (int32_t)value;
		break;
	case 26:
		m_projectionPlaneDistance = (uint16_t)value;
		break;
	case 27:
		m_depthQueingCoefficient = (int16_t)value;
		break;
	case 28:
		m_depthQueingOffset = (int32_t)value;
		break;
	case 29:
		m_zScaleFactor3 = (int16_t)value;
		break;
	case 30:
		m_zScaleFactor4 = (int16_t)value;
		break;
	default:
		assert(0, "Unhandled GTE control register");
		break;
	}
}

void Gte::setData(uint32_t reg, uint32_t value)
{
	LOG("Set GTE data reg = 0x" << std::hex << reg << " value = 0x" << value);

	switch (reg)
	{
	case 0:
		m_vector0[0] = (int16_t)value;
		m_vector0[1] = (int16_t)(value >> 16);
		break;
	case 1:
		m_vector0[2] = (int16_t)value;
		break;
	case 2:
		m_vector1[0] = (int16_t)value;
		m_vector1[1] = (int16_t)(value >> 16);
		break;
	case 3:
		m_vector1[2] = (int16_t)value;
		break;
	case 4:
		m_vector2[0] = (int16_t)value;
		m_vector2[1] = (int16_t)(value >> 16);
		break;
	case 5:
		m_vector2[2] = (int16_t)value;
		break;
	default:
		assert(0, "Unhandled GTE data register");
		break;
	}
}

void Gte::command(uint32_t command)
{
	assert(0, "Unhandled GTE command");
}
