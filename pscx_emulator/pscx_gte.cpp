#include <cassert>

#include "pscx_gte.h"

void Gte::setControl(uint32_t reg, uint32_t value)
{
	switch (reg)
	{
	case 13:
		m_redBackgroundColor = (int32_t)value;
		break;
	case 14:
		m_greenBackgroundColor = (int32_t)value;
		break;
	case 15:
		m_blueBackgroundColor = (int32_t)value;
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
