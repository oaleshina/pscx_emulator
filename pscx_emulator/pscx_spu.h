#pragma once

#include <cstdint>
#include <memory.h>

namespace regmap 
{
	// SPU register map: offset from the base 
	// in number of halfwords.

	namespace voice
	{
		// Per-voice regmap, repeated 24 times.
		const size_t VOLUME_LEFT = 0x0;
		const size_t VOLUME_RIGHT = 0x1;
		const size_t ADPCM_SAMPLE_RATE = 0x2;
		const size_t ADPCM_START_INDEX = 0x3;
		const size_t ADPCM_ADSR_LOW = 0x4;
		const size_t ADPCM_ADSR_HIGH = 0x5;
		const size_t CURRENT_ADSR_VOLUME = 0x6;
		const size_t ADPCM_REPEAT_INDEX = 0x7;
	}

	const size_t MAIN_VOLUME_LEFT = 0xc0;
	const size_t MAIN_VOLUME_RIGHT = 0xc1;
	const size_t REVERB_VOLUME_LEFT = 0xc2;
	const size_t REVERB_VOLUME_RIGHT = 0xc3;
	const size_t VOICE_ON_LOW = 0xc4;
	const size_t VOICE_ON_HIGH = 0xc5;
	const size_t VOICE_OFF_LOW = 0xc6;
	const size_t VOICE_OFF_HIGH = 0xc7;
	const size_t VOICE_PITCH_MOD_EN_LOW = 0xc8;
	const size_t VOICE_PITCH_MOD_EN_HIGH = 0xc9;
	const size_t VOICE_NOISE_EN_LOW = 0xca;
	const size_t VOICE_NOISE_EN_HIGH = 0xcb;
	const size_t VOICE_REVERB_EN_LOW = 0xcc;
	const size_t VOICE_REVERB_EN_HIGH = 0xcd;
	const size_t VOICE_STATUS_LOW = 0xce;
	const size_t VOICE_STATUS_HIGH = 0xcf;

	const size_t REVERB_BASE = 0xd1;
	const size_t TRANSFER_START_INDEX = 0xd3;
	const size_t TRANSFER_FIFO = 0xd4;
	const size_t CONTROL = 0xd5;
	const size_t TRANSFER_CONTROL = 0xd6;
	const size_t STATUS = 0xd7;
	const size_t CD_VOLUME_LEFT = 0xd8;
	const size_t CD_VOLUME_RIGHT = 0xd9;
	const size_t EXT_VOLUME_LEFT = 0xda;
	const size_t EXT_VOLUME_RIGHT = 0xdb;
	const size_t CURRENT_VOLUME_LEFT = 0xdc;
	const size_t CURRENT_VOLUME_RIGHT = 0xdd;

	const size_t REVERB_APF_OFFSET1 = 0xe0;
	const size_t REVERB_APF_OFFSET2 = 0xe1;
	const size_t REVERB_REFLECT_VOLUME1 = 0xe2;
	const size_t REVERB_COMB_VOLUME1 = 0xe3;
	const size_t REVERB_COMB_VOLUME2 = 0xe4;
	const size_t REVERB_COMB_VOLUME3 = 0xe5;
	const size_t REVERB_COMB_VOLUME4 = 0xe6;
	const size_t REVERB_REFLECT_VOLUME2 = 0xe7;
	const size_t REVERB_APF_VOLUME1 = 0xe8;
	const size_t REVERB_APF_VOLUME2 = 0xe9;
	const size_t REVERB_REFLECT_SAME_LEFT1 = 0xea;
	const size_t REVERB_REFLECT_SAME_RIGHT1 = 0xeb;
	const size_t REVERB_COMB_LEFT1 = 0xec;
	const size_t REVERB_COMB_RIGHT1 = 0xed;
	const size_t REVERB_COMB_LEFT2 = 0xee;
	const size_t REVERB_COMB_RIGHT2 = 0xef;
	const size_t REVERB_REFLECT_SAME_LEFT2 = 0xf0;
	const size_t REVERB_REFLECT_SAME_RIGHT2 = 0xf1;
	const size_t REVERB_REFLECT_DIFF_LEFT1 = 0xf2;
	const size_t REVERB_REFLECT_DIFF_RIGHT1 = 0xf3;
	const size_t REVERB_COMB_LEFT3 = 0xf4;
	const size_t REVERB_COMB_RIGHT3 = 0xf5;
	const size_t REVERB_COMB_LEFT4 = 0xf6;
	const size_t REVERB_COMB_RIGHT4 = 0xf7;
	const size_t REVERB_REFLECT_DIFF_LEFT2 = 0xf8;
	const size_t REVERB_REFLECT_DIFF_RIGHT2 = 0xf9;
	const size_t REVERB_APF_LEFT1 = 0xfa;
	const size_t REVERB_APF_RIGHT1 = 0xfb;
	const size_t REVERB_APF_LEFT2 = 0xfc;
	const size_t REVERB_APF_RIGHT2 = 0xfd;
	const size_t REVERB_INPUT_VOLUME_LEFT = 0xfe;
	const size_t REVERB_INPUT_VOLUME_RIGHT = 0xff;
}

// Sound Processing Unit
struct Spu
{
	Spu()
	{
		memset(m_shadowRegisters, 0x0, sizeof(m_shadowRegisters));
		for (size_t i = 0; i < 256 * 1024; ++i) m_ram[i] = 0xbad;
		m_ramIndex = 0x0;
	}

	template<typename T>
	void store(uint32_t offset, T value);

	template<typename T>
	T load(uint32_t offset);

	uint16_t getControl() const { return m_shadowRegisters[regmap::CONTROL]; };
	uint16_t getStatus() const { return getControl() & 0x3f; }

	void setControl(uint16_t value);

	// Set the SPU RAM access pattern.
	void setTransferControl(uint16_t value);
	void fifoWrite(uint16_t value);

private:
	// Most of the SPU registers aren't updated by the hardware,
	// their value is just moved to the internal registers when needed.
	// Therefore we can emulate those registers like a RAM of sorts.
	uint16_t m_shadowRegisters[0x100];

	// SPU RAM: 256k 16bit samples.
	uint16_t m_ram[256 * 1024];
	// Write pointer in the SPU RAM.
	uint32_t m_ramIndex;
};

