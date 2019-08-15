#include <cassert>

#include "pscx_spu.h"
#include "pscx_common.h"

template<typename T>
void Spu::store(uint32_t offset, T value)
{
	assert((std::is_same<uint16_t, T>::value), "Unhandled SPU store");

	// Convert into a halfword index.
	size_t index = offset >> 1;

	if (index < 0xc0)
	{
		switch (index & 0x7)
		{
		case regmap::voice::VOLUME_LEFT: // => ()
		case regmap::voice::VOLUME_RIGHT:
		case regmap::voice::ADPCM_SAMPLE_RATE:
		case regmap::voice::ADPCM_START_INDEX:
		case regmap::voice::ADPCM_ADSR_LOW:
		case regmap::voice::ADPCM_ADSR_HIGH:
		case regmap::voice::CURRENT_ADSR_VOLUME:
		case regmap::voice::ADPCM_REPEAT_INDEX:
			break;
		default:
			assert(0, "Unreachable!");
		}
	}
	else
	{
		switch (index)
		{
		case regmap::MAIN_VOLUME_LEFT:
		case regmap::MAIN_VOLUME_RIGHT:
		case regmap::REVERB_VOLUME_LEFT:
		case regmap::REVERB_VOLUME_RIGHT:
			break;
		case regmap::VOICE_ON_LOW:
			m_shadowRegisters[regmap::VOICE_STATUS_LOW] |= value;
			break;
		case regmap::VOICE_ON_HIGH:
			m_shadowRegisters[regmap::VOICE_STATUS_HIGH] |= value;
			break;
		case regmap::VOICE_OFF_LOW:
			m_shadowRegisters[regmap::VOICE_STATUS_LOW] &= ~value;
			break;
		case regmap::VOICE_OFF_HIGH:
			m_shadowRegisters[regmap::VOICE_STATUS_HIGH] &= ~value;
			break;
		case regmap::VOICE_PITCH_MOD_EN_LOW:
		case regmap::VOICE_PITCH_MOD_EN_HIGH:
		case regmap::VOICE_NOISE_EN_LOW:
		case regmap::VOICE_NOISE_EN_HIGH:
		case regmap::VOICE_REVERB_EN_LOW:
		case regmap::VOICE_REVERB_EN_HIGH:
		case regmap::VOICE_STATUS_LOW:
		case regmap::VOICE_STATUS_HIGH:
		case regmap::REVERB_BASE:
			break;
		case regmap::TRANSFER_START_INDEX:
			m_ramIndex = (uint32_t)value << 2;
			break;
		case regmap::TRANSFER_FIFO:
			fifoWrite(value);
			break;
		case regmap::CONTROL:
			setControl(value);
			break;
		case regmap::TRANSFER_CONTROL:
			setTransferControl(value);
			break;
		case regmap::CD_VOLUME_LEFT:
		case regmap::CD_VOLUME_RIGHT:
		case regmap::EXT_VOLUME_LEFT:
		case regmap::EXT_VOLUME_RIGHT:
		case regmap::REVERB_APF_OFFSET1:
		case regmap::REVERB_APF_OFFSET2:
		case regmap::REVERB_REFLECT_VOLUME1:
		case regmap::REVERB_COMB_VOLUME1:
		case regmap::REVERB_COMB_VOLUME2:
		case regmap::REVERB_COMB_VOLUME3:
		case regmap::REVERB_COMB_VOLUME4:
		case regmap::REVERB_REFLECT_VOLUME2:
		case regmap::REVERB_APF_VOLUME1:
		case regmap::REVERB_APF_VOLUME2:
		case regmap::REVERB_REFLECT_SAME_LEFT1:
		case regmap::REVERB_REFLECT_SAME_RIGHT1:
		case regmap::REVERB_COMB_LEFT1:
		case regmap::REVERB_COMB_RIGHT1:
		case regmap::REVERB_COMB_LEFT2:
		case regmap::REVERB_COMB_RIGHT2:
		case regmap::REVERB_REFLECT_SAME_LEFT2:
		case regmap::REVERB_REFLECT_SAME_RIGHT2:
		case regmap::REVERB_REFLECT_DIFF_LEFT1:
		case regmap::REVERB_REFLECT_DIFF_RIGHT1:
		case regmap::REVERB_COMB_LEFT3:
		case regmap::REVERB_COMB_RIGHT3:
		case regmap::REVERB_COMB_LEFT4:
		case regmap::REVERB_COMB_RIGHT4:
		case regmap::REVERB_REFLECT_DIFF_LEFT2:
		case regmap::REVERB_REFLECT_DIFF_RIGHT2:
		case regmap::REVERB_APF_LEFT1:
		case regmap::REVERB_APF_RIGHT1:
		case regmap::REVERB_APF_LEFT2:
		case regmap::REVERB_APF_RIGHT2:
		case regmap::REVERB_INPUT_VOLUME_LEFT:
		case regmap::REVERB_INPUT_VOLUME_RIGHT:
			break;
		default:
			assert(0, "Unhandled SPU store");
		}
	}

	if (index < 0x100)
		m_shadowRegisters[index] = value;
}

template void Spu::store<uint32_t>(uint32_t, uint32_t);
template void Spu::store<uint16_t>(uint32_t, uint16_t);
template void Spu::store<uint8_t >(uint32_t, uint8_t );

template<typename T>
T Spu::load(uint32_t offset)
{
	assert((std::is_same<uint16_t, T>::value), "Unhandled SPU load");

	size_t index = offset >> 1;

	uint16_t shadowRegister = 0x0;
	if (index < 0xc0)
	{
		switch (index & 0x7)
		{
		case regmap::voice::CURRENT_ADSR_VOLUME:
		case regmap::voice::ADPCM_REPEAT_INDEX:
		default:
			shadowRegister = m_shadowRegisters[index];
			break;
		}
	}
	else
	{
		switch (index)
		{
		case regmap::VOICE_ON_LOW:
		case regmap::VOICE_ON_HIGH:
		case regmap::VOICE_OFF_LOW:
		case regmap::VOICE_OFF_HIGH:
		case regmap::VOICE_REVERB_EN_LOW:
		case regmap::VOICE_REVERB_EN_HIGH:
		case regmap::VOICE_STATUS_LOW:
		case regmap::VOICE_STATUS_HIGH:
		case regmap::TRANSFER_START_INDEX:
		case regmap::CONTROL:
		case regmap::TRANSFER_CONTROL:
			shadowRegister = m_shadowRegisters[index];
			break;
		case regmap::STATUS:
			shadowRegister = getStatus();
			break;
		case regmap::CURRENT_VOLUME_LEFT:
		case regmap::CURRENT_VOLUME_RIGHT:
			shadowRegister = m_shadowRegisters[index];
			break;
		default:
			assert(0, "Unhandled SPU load");
		}
	}

	return shadowRegister;
}

template uint32_t Spu::load<uint32_t>(uint32_t);
template uint16_t Spu::load<uint16_t>(uint32_t);
template uint8_t  Spu::load<uint8_t >(uint32_t);

void Spu::setControl(uint16_t value)
{
	assert((value & 0x3f4a) == 0x0, "Unhandled SPU control");
}


void Spu::setTransferControl(uint16_t value)
{
	assert(value == 0x4, "Unhandled SPU RAM access pattern");
}

void Spu::fifoWrite(uint16_t value)
{
	LOG("SPU RAM store 0x" << std::hex << m_ramIndex << " 0x" << value);
	m_ram[m_ramIndex] = value;
	m_ramIndex = (m_ramIndex + 1) & 0x3ffff;
}
