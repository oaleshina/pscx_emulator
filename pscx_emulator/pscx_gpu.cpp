#include "pscx_gpu.h"

template<typename T>
T Gpu::load(uint32_t offset) const
{
	if (!std::is_same<uint32_t, T>::value)
	{
		LOG("Unhandled GPU load");
		return ~0;
	}

	// GPUSTAT: set bit 26, 27, 28 to signal that the GPU is ready for DMA and CPU access.
	// This way the BIOS won't dead lock waiting for an event that will never come
	if (offset == 0x0)
		return getReadRegister();
	if (offset == 0x4)
		return getStatusRegister();
	
	return 0;
}

template uint32_t Gpu::load<uint32_t>(uint32_t) const;
template uint16_t Gpu::load<uint16_t>(uint32_t) const;
template uint8_t  Gpu::load<uint8_t> (uint32_t) const;

template<typename T>
void Gpu::store(uint32_t offset, T value)
{
	if (!std::is_same<uint32_t, T>::value)
	{
		LOG("Unhandled GPU store");
		return;
	}

	if (offset == 0x0)
		gp0(value);
	else if (offset == 0x4)
		gp1(value);
	else
		LOG("GPU write 0x" << std::hex << offset << " 0x" << value);
	return;
}

template void Gpu::store<uint32_t>(uint32_t, uint32_t);
template void Gpu::store<uint16_t>(uint32_t, uint16_t);
template void Gpu::store<uint8_t> (uint32_t, uint8_t );

uint32_t Gpu::getStatusRegister() const
{
	uint32_t statusRegister = 0;

	statusRegister |= m_pageBaseX;
	statusRegister |= ((uint32_t)m_pageBaseY) << 4;
	statusRegister |= ((uint32_t)m_semiTransparency) << 5;
	statusRegister |= ((uint32_t)m_textureDepth) << 7;
	statusRegister |= ((uint32_t)m_dithering) << 9;
	statusRegister |= ((uint32_t)m_drawToDisplay) << 10;
	statusRegister |= ((uint32_t)m_forceSetMaskBit) << 11;
	statusRegister |= ((uint32_t)m_preserveMaskedPixels) << 12;
	statusRegister |= ((uint32_t)m_field) << 13;
	// Bit 14: not supported
	statusRegister |= ((uint32_t)m_textureDisable) << 15;
	statusRegister |= m_hres.intoStatus();
	// Temporary hack: if we don't emulate bit 31 correctly,
	// setting 'vres' to 1 locks the BIOS:
	// statusRegister |= ((uint32_t)m_vres) << 19;
	statusRegister |= ((uint32_t)m_vmode) << 20;
	statusRegister |= ((uint32_t)m_displayDepth) << 21;
	statusRegister |= ((uint32_t)m_interlaced) << 22;
	statusRegister |= ((uint32_t)m_displayDisabled) << 23;
	statusRegister |= ((uint32_t)m_interrupt) << 24;

	// For now we pretend that the GPU is always ready:
	// Ready to receive command
	statusRegister |= 1 << 26;
	// Ready to send VRAM to CPU
	statusRegister |= 1 << 27;
	// Ready to receive DMA block
	statusRegister |= 1 << 28;

	statusRegister |= ((uint32_t)m_dmaDirection) << 29;

	// Bit 31 should change depending on currently drawn line ( whether it's even or odd in the vblack apparantly )
	statusRegister |= 0 << 31;

	// It's the signal checked by the DMA when sending data in Request synchronization mode
	uint32_t dmaRequest = 0;
	switch (m_dmaDirection)
	{
	// Always 0
	case DmaDirection::DMA_DIRECTION_OFF:
		dmaRequest = 0;
		break;
	// Should be 0 if FIFO is full, 1 otherwise
	case DmaDirection::DMA_DIRECTION_FIFO:
		dmaRequest = 1;
		break;
	// Should be the same as status bit 28
	case DmaDirection::DMA_DIRECTION_CPU_TO_GP0:
		dmaRequest = (statusRegister >> 28) & 1;
		break;
	// Should be the same as status bit 27
	case DmaDirection::DMA_DIRECTION_VRAM_TO_CPU:
		dmaRequest = (statusRegister >> 27) & 1;
		break;
	}

	statusRegister |= dmaRequest << 25;
	return statusRegister;
}

uint32_t Gpu::getReadRegister() const
{
	return 0x0;
}

void Gpu::gp0(uint32_t value)
{
	if (m_gp0WordsRemaining == 0)
	{
		// We start a new GP0 command
		uint32_t opcode = (value >> 24) & 0xff;

		struct CommandParameters
		{
			uint32_t gp0WordsRemaining;
			void(Gpu::*gp0CommandMethod)(void);
		};

		CommandParameters commandParameters;
		switch (opcode)
		{
		case 0x0:
			commandParameters.gp0WordsRemaining = 1;
			commandParameters.gp0CommandMethod = &Gpu::gp0Nop;
			break;
		case 0x01:
			commandParameters.gp0WordsRemaining = 1;
			commandParameters.gp0CommandMethod = &Gpu::gp0ClearCache;
			break;
		case 0x28:
			commandParameters.gp0WordsRemaining = 5;
			commandParameters.gp0CommandMethod = &Gpu::gp0QuadMonoOpaque;
			break;
		case 0x2c:
			commandParameters.gp0WordsRemaining = 9;
			commandParameters.gp0CommandMethod = &Gpu::gp0QuadTextureBlendOpaque;
			break;
		case 0x30:
			commandParameters.gp0WordsRemaining = 6;
			commandParameters.gp0CommandMethod = &Gpu::gp0TriangleShadedOpaque;
			break;
		case 0x38:
			commandParameters.gp0WordsRemaining = 8;
			commandParameters.gp0CommandMethod = &Gpu::gp0QuadShadedOpaque;
			break;
		case 0xa0:
			commandParameters.gp0WordsRemaining = 3;
			commandParameters.gp0CommandMethod = &Gpu::gp0ImageLoad;
			break;
		case 0xc0:
			commandParameters.gp0WordsRemaining = 3;
			commandParameters.gp0CommandMethod = &Gpu::gp0ImageStore;
			break;
		case 0xe1:
			commandParameters.gp0WordsRemaining = 1;
			commandParameters.gp0CommandMethod = &Gpu::gp0DrawMode;
			break;
		case 0xe2:
			commandParameters.gp0WordsRemaining = 1;
			commandParameters.gp0CommandMethod = &Gpu::gp0TextureWindow;
			break;
		case 0xe3:
			commandParameters.gp0WordsRemaining = 1;
			commandParameters.gp0CommandMethod = &Gpu::gp0DrawingAreaTopLeft;
			break;
		case 0xe4:
			commandParameters.gp0WordsRemaining = 1;
			commandParameters.gp0CommandMethod = &Gpu::gp0DrawingAreaBottomRight;
			break;
		case 0xe5:
			commandParameters.gp0WordsRemaining = 1;
			commandParameters.gp0CommandMethod = &Gpu::gp0DrawingOffset;
			break;
		case 0xe6:
			commandParameters.gp0WordsRemaining = 1;
			commandParameters.gp0CommandMethod = &Gpu::gp0MaskBitSetting;
			break;
		default:
			LOG("Unhandled GP0 command 0x" << std::hex << value);
		}

		m_gp0WordsRemaining = commandParameters.gp0WordsRemaining;
		m_gp0CommandMethod = commandParameters.gp0CommandMethod;

		m_gp0Command.clear();
	}

	m_gp0WordsRemaining -= 1;

	switch (m_gp0Mode)
	{
	case Gp0Mode::GP0_MODE_COMMAND:
		m_gp0Command.pushWord(value);
		if (m_gp0WordsRemaining == 0)
		{
			// We have all the parameters, we can run the command
			(this->*m_gp0CommandMethod)();
		}
		break;
	case Gp0Mode::GP0_MODE_IMAGE_LOAD:
		if (m_gp0WordsRemaining == 0)
		{
			// Load done, switch back to command mode
			m_gp0Mode = Gp0Mode::GP0_MODE_COMMAND;
		}
	}
}

void Gpu::gp1(uint32_t value)
{
	uint32_t opcode = (value >> 24) & 0xff;

	switch (opcode)
	{
	case 0x0:
		gp1Reset(value);
		break;
	case 0x01:
		gp1ResetCommandBuffer();
		break;
	case 0x02:
		gp1AcknowledgeIrq();
		break;
	case 0x03:
		gp1DisplayEnable(value);
		break;
	case 0x04:
		gp1DmaDirection(value);
		break;
	case 0x05:
		gp1DisplayVramStart(value);
		break;
	case 0x06:
		gp1DisplayHorizontalRange(value);
		break;
	case 0x07:
		gp1DisplayVerticalRange(value);
		break;
	case 0x08:
		gp1DisplayMode(value);
		break;
	default:
		LOG("Unhandled GP1 command 0x" << std::hex << value);
	}
}

void Gpu::gp0Nop()
{
	// NOP
}

void Gpu::gp0ClearCache()
{
	// Not implemented
}

void Gpu::gp0QuadMonoOpaque()
{
	Position positions[] = {
		Position::fromGP0(m_gp0Command[1]),
		Position::fromGP0(m_gp0Command[2]),
		Position::fromGP0(m_gp0Command[3]),
		Position::fromGP0(m_gp0Command[4])
	};

	// Only one color repeated 4 times
	Color colors[] = {
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0])
	};

	m_renderer.pushQuad(positions, colors);
}

void Gpu::gp0QuadTextureBlendOpaque()
{
	Position positions[] = {
		Position::fromGP0(m_gp0Command[1]),
		Position::fromGP0(m_gp0Command[3]),
		Position::fromGP0(m_gp0Command[5]),
		Position::fromGP0(m_gp0Command[7])
	};

	// We don't support textures for now, use the solid red color instead
	Color colors[] = {
		Color(0x80, 0x0, 0x0),
		Color(0x80, 0x0, 0x0),
		Color(0x80, 0x0, 0x0),
		Color(0x80, 0x0, 0x0)
	};

	m_renderer.pushQuad(positions, colors);
}

void Gpu::gp0TriangleShadedOpaque()
{
	Position positions[] = {
		Position::fromGP0(m_gp0Command[1]),
		Position::fromGP0(m_gp0Command[3]),
		Position::fromGP0(m_gp0Command[5])
	};

	Color colors[] = {
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[2]),
		Color::fromGP0(m_gp0Command[4])
	};

	m_renderer.pushTriangle(positions, colors);
}

void Gpu::gp0QuadShadedOpaque()
{
	Position positions[] = {
	Position::fromGP0(m_gp0Command[1]),
	Position::fromGP0(m_gp0Command[3]),
	Position::fromGP0(m_gp0Command[5]),
	Position::fromGP0(m_gp0Command[7])
	};

	// Only one color repeated 4 times
	Color colors[] = {
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[2]),
		Color::fromGP0(m_gp0Command[4]),
		Color::fromGP0(m_gp0Command[6])
	};

	m_renderer.pushQuad(positions, colors);
}

void Gpu::gp0ImageLoad()
{
	// Parameter 2 contains the image resolution
	uint32_t imageResolution = m_gp0Command[2];

	uint32_t width = imageResolution & 0xffff;
	uint32_t height = imageResolution >> 16;

	// Size of the image in 16 bit pixels
	uint32_t imageSize = width * height;

	// If we have an odd number of pixels we must round up
	// since we transfer 32 bits at a time. There will be 16 bits
	// of padding in the last word
	imageSize = (imageSize + 1) & ~1;

	// Store number of words expected for this image
	m_gp0WordsRemaining = imageSize / 2;

	// Put the GP0 state machine in the ImageLoad mode
	m_gp0Mode = Gp0Mode::GP0_MODE_IMAGE_LOAD;
}

void Gpu::gp0ImageStore()
{
	// Parameter 2 contains the image resolution
	uint32_t imageResolution = m_gp0Command[2];

	uint32_t width = imageResolution & 0xffff;
	uint32_t height = imageResolution >> 16;

	LOG("Unhandled image store 0x" << width << " 0x" << height);
}

void Gpu::gp0DrawMode()
{
	uint32_t value = m_gp0Command[0];

	m_pageBaseX = value & 0xf;
	m_pageBaseY = (value >> 4) & 1;
	m_semiTransparency = (value >> 5) & 3;

	TextureDepth textureDepth;
	uint32_t textureDepthValue = (value >> 7) & 3;
	switch (textureDepthValue)
	{
	case 0:
		textureDepth = TextureDepth::TEXTURE_DEPTH_4_BIT;
		break;
	case 1:
		textureDepth = TextureDepth::TEXTURE_DEPTH_8_BIT;
		break;
	case 2:
		textureDepth = TextureDepth::TEXTURE_DEPTH_15_BIT;
		break;
	default:
		LOG("Unhandled texture depth 0x" << std::hex << textureDepthValue);
		return;
	}

	m_textureDepth = textureDepth;
	m_dithering = (value >> 9) & 1;
	m_drawToDisplay = (value >> 10) & 1;
	m_textureDisable = (value >> 11) & 1;
	m_rectangleTextureXFlip = (value >> 12) & 1;
	m_rectangleTextureYFlip = (value >> 13) & 1;
}

void Gpu::gp0DrawingAreaTopLeft()
{
	uint32_t value = m_gp0Command[0];
	m_drawingAreaTop = (value >> 10) & 0x3ff;
	m_drawingAreaLeft = value & 0x3ff;
}

void Gpu::gp0DrawingAreaBottomRight()
{
	uint32_t value = m_gp0Command[0];
	m_drawingAreaBottom = (value >> 10) & 0x3ff;
	m_drawingAreaRight = value & 0x3ff;
}

void Gpu::gp0DrawingOffset()
{
	uint32_t value = m_gp0Command[0];
	uint16_t x = value & 0x7ff;
	uint16_t y = (value >> 11) & 0x7ff;

	// Values are 11 bit two's complement signed values, we need to shift
	// the value to 16 bit to force sign extension
	int16_t offsetX = ((int16_t)(x << 5)) >> 5;
	int16_t offsetY = ((int16_t)(y << 5)) >> 5;

	m_renderer.setDrawOffset(offsetX, offsetY);
	m_renderer.display();
}

void Gpu::gp0TextureWindow()
{
	uint32_t value = m_gp0Command[0];
	m_textureWindowXMask = value & 0x1f;
	m_textureWindowYMask = (value >> 5) & 0x1f;
	m_textureWindowXOffset = (value >> 10) & 0x1f;
	m_textureWindowYOffset = (value >> 15) & 0x1f;
}

void Gpu::gp0MaskBitSetting()
{
	uint32_t value = m_gp0Command[0];
	m_forceSetMaskBit = value & 1;
	m_preserveMaskedPixels = value & 2;
}

void Gpu::gp1Reset(uint32_t value)
{
	m_interrupt = false;

	m_pageBaseX = 0x0;
	m_pageBaseY = 0x0;
	m_semiTransparency = 0x0;
	m_textureDepth = TextureDepth::TEXTURE_DEPTH_4_BIT;
	m_textureWindowXMask = 0x0;
	m_textureWindowYMask = 0x0;
	m_textureWindowXOffset = 0x0;
	m_textureWindowYOffset = 0x0;
	m_dithering = false;
	m_drawToDisplay = false;
	m_textureDisable = false;
	m_rectangleTextureXFlip = false;
	m_rectangleTextureYFlip = false;
	m_drawingAreaLeft = 0x0;
	m_drawingAreaTop = 0x0;
	m_drawingAreaRight = 0x0;
	m_drawingAreaBottom = 0x0;
	m_forceSetMaskBit = false;
	m_preserveMaskedPixels = false;

	m_dmaDirection = DmaDirection::DMA_DIRECTION_OFF;

	m_displayDisabled = true;
	m_displayVramXStart = 0x0;
	m_displayVramYStart = 0x0;
	m_hres = HorizontalRes::createFromFields(0, 0);
	m_vres = VerticalRes::VERTICAL_RES_240_LINES;

	m_vmode = VMode::VMODE_NTSC;
	m_interlaced = true;
	m_displayHorizStart = 0x200;
	m_displayHorizEnd = 0xc00;
	m_displayLineStart = 0x10;
	m_displayLineEnd = 0x100;
	m_displayDepth = DisplayDepth::DISPLAY_DEPTH_15_BITS;

	m_renderer.setDrawOffset(0, 0);

	gp1ResetCommandBuffer();
	gp1AcknowledgeIrq();
}

void Gpu::gp1ResetCommandBuffer()
{
	m_gp0Command.clear();
	m_gp0WordsRemaining = 0x0;
	m_gp0Mode = Gp0Mode::GP0_MODE_COMMAND;
}

void Gpu::gp1AcknowledgeIrq()
{
	m_interrupt = false;
}

void Gpu::gp1DisplayEnable(uint32_t value)
{
	m_displayDisabled = value & 1;
}

void Gpu::gp1DisplayMode(uint32_t value)
{
	uint8_t hr1 = value & 3;
	uint8_t hr2 = (value >> 6) & 1;
	
	m_hres = HorizontalRes::createFromFields(hr1, hr2);

	m_vres = VerticalRes::VERTICAL_RES_240_LINES;
	if (value & 0x4)
		m_vres = VerticalRes::VERTICAL_RES_480_LINES;
	
	m_vmode = VMode::VMODE_NTSC;
	if (value & 0x8)
		m_vmode = VMode::VMODE_PAL;
	
	m_displayDepth = DisplayDepth::DISPLAY_DEPTH_24_BITS;
	if (value & 0x10)
		m_displayDepth = DisplayDepth::DISPLAY_DEPTH_15_BITS;

	m_interlaced = value & 0x20;

	if (value & 0x80)
		LOG("Unsupported display mode 0x" << std::hex << val);
}

void Gpu::gp1DmaDirection(uint32_t value)
{
	switch (value & 0x3)
	{
	case 0:
		m_dmaDirection = DmaDirection::DMA_DIRECTION_OFF;
		break;
	case 1:
		m_dmaDirection = DmaDirection::DMA_DIRECTION_FIFO;
		break;
	case 2:
		m_dmaDirection = DmaDirection::DMA_DIRECTION_CPU_TO_GP0;
		break;
	case 3:
		m_dmaDirection = DmaDirection::DMA_DIRECTION_VRAM_TO_CPU;
		break;
	}
}

void Gpu::gp1DisplayVramStart(uint32_t value)
{
	m_displayVramXStart = value & 0x3fe;
	m_displayVramYStart = (value >> 10) & 0x1ff;
}

void Gpu::gp1DisplayHorizontalRange(uint32_t value)
{
	m_displayHorizStart = value & 0xfff;
	m_displayHorizEnd = (value >> 12) & 0xfff;
}

void Gpu::gp1DisplayVerticalRange(uint32_t value)
{
	m_displayLineStart = value & 0x3ff;
	m_displayLineEnd = (value >> 10) & 0x3ff;
}
