#include <cassert>

#include "pscx_gpu.h"
#include "pscx_cpu.h"

std::pair<uint16_t, uint16_t> Gpu::getVModeTimings() const
{
	// The number of ticks per line is an estimate using the
	// average line length recorded by the timer1 using the
	// "hsync" clock source.
	if (m_vmode == VMode::VMODE_NTSC)
		return std::make_pair(3412, 263);
	return std::make_pair(3404, 314); // VMode::Pal
}

FracCycles Gpu::gpuToCpuClockRatio() const
{
	// First we convert the delta into GPU clock periods.
	// GPU clock in Hz
	uint32_t gpuClock = 53'200'000; // HardwareType::HARDWARE_TYPE_PAL
	if (m_hardwareType == HardwareType::HARDWARE_TYPE_NTSC)
		gpuClock = 53'690'000;

	// Clock ratio shifted 16 bits to the left
	return FracCycles::fromF32(gpuClock / (float)CPU_FREQ_HZ);
}

FracCycles Gpu::dotclockPeriod()
{
	FracCycles gpuClockPeriod = gpuToCpuClockRatio();
	uint8_t dotclockDivider = m_hres.dotclockDivider();

	// Dividing the clock frequency means multiplying its period
	Cycles period = gpuClockPeriod.getFp() * (Cycles)dotclockDivider;
	return FracCycles::fromFp(period);
}

FracCycles Gpu::dotclockPhase()
{
	//return FracCycles::fromCycles(m_gpuClockPhase);
	assert(0, "GPU dotclock phase is not implemented");
	return 0;
}

FracCycles Gpu::hsyncPeriod()
{
	std::pair<uint16_t, uint16_t> vModeTimings = getVModeTimings();
	uint16_t ticksPerLine = vModeTimings.first;

	FracCycles lineLen = FracCycles::fromCycles(ticksPerLine);

	// Convert from GPU cycles into CPU cycles
	return lineLen.divide(gpuToCpuClockRatio());
}

FracCycles Gpu::hsyncPhase()
{
	FracCycles phase = FracCycles::fromCycles(m_displayLineTick);
	FracCycles clockPhase = FracCycles::fromFp(m_gpuClockPhase);

	phase = phase.add(clockPhase);

	// Convert phase from GPU clock cycles into CPU clock cycles
	return phase.multiply(gpuToCpuClockRatio());
}

void Gpu::sync(TimeKeeper& timeKeeper, InterruptState& irqState)
{
	Cycles delta = timeKeeper.sync(Peripheral::PERIPHERAL_GPU);

	// Convert delta in GPU time, adding the leftover from the last time
	delta = (Cycles)m_gpuClockPhase + delta * gpuToCpuClockRatio().getFp();

	// The 16 low bits are the new fractional part
	m_gpuClockPhase = delta;

	// Convert delta back to integer
	delta >>= 16;

	// Compute the current line and posiition within the line
	std::pair<uint16_t, uint16_t> vModeTimings = getVModeTimings();

	Cycles ticksPerLine = vModeTimings.first;
	Cycles linesPerFrame = vModeTimings.second;

	Cycles lineTick = (Cycles)m_displayLineTick + delta;
	Cycles line = (Cycles)m_displayLine + lineTick / ticksPerLine;

	m_displayLineTick = lineTick % ticksPerLine;

	m_displayLine = line;
	if (line > linesPerFrame)
	{
		// New frame
		if (m_interlaced)
		{
			// Update the field
			Cycles nframes = line / linesPerFrame;

			m_field = Field::FIELD_BOTTOM;
			if ((nframes + (Cycles)m_field) & 1)
				m_field = Field::FIELD_TOP;
		}
		m_displayLine = line % linesPerFrame;
	}

	bool vblankInterrupt = inVblank();
	if (!m_vblankInterrupt && vblankInterrupt)
	{
		// Rising edge of the vblank interrupt
		irqState.raiseAssert(Interrupt::INTERRUPT_VBLANK);
	}

	if (m_vblankInterrupt && !vblankInterrupt)
	{
		// End of vertical blanking, probably as a good place as
		// any to update the display
		m_renderer.display();
	}

	m_vblankInterrupt = vblankInterrupt;
	predictNextSync(timeKeeper);
}

void Gpu::predictNextSync(TimeKeeper timeKeeper)
{
	std::pair<uint16_t, uint16_t> vModeTimings = getVModeTimings();

	Cycles ticksPerLine = vModeTimings.first;
	Cycles linesPerFrame = vModeTimings.second;

	Cycles delta = 0;

	Cycles currentLine = m_displayLine;

	Cycles displayLineStart = m_displayLineStart;
	Cycles displayLineEnd = m_displayLineEnd;

	// Number of ticks to get to the start of the next line
	delta += (ticksPerLine - (Cycles)m_displayLineTick);

	// The various -1 in the next formulas are because we start
	// counting at line 0. Without them we'd go one line too far.
	if (currentLine >= displayLineEnd)
	{
		// We're in the vertical blanking at the end of the frame.
		// We want to synchronize at the end of the blanking at the
		// beginning of the next frame.

		// Number of ticks to get to the end of the frame
		delta += (linesPerFrame - currentLine) * ticksPerLine;

		// Number of ticks to get to the end og vblank in the next frame
		delta += (displayLineStart - 1) * ticksPerLine;
	}
	else if (currentLine < displayLineStart)
	{
		// We're in the verical blanking at the beginning of the frame.
		// We want to synchronize at the end of the blanking for the current frame.

		delta += (displayLineStart - 1 - currentLine) * ticksPerLine;
	}
	else
	{
		// We're in the active video, we want to synchronize at the beginning
		// of the vertical blanking period.
		delta += (displayLineEnd - 1 - currentLine) * ticksPerLine;
	}

	// Convert delta in CPU clock periods.
	delta <<= FracCycles::fracBits(); //*= CLOCK_RATIO_FRAC;
	// Remove the current fractiona; cycle to be more accurate.
	delta -= (Cycles)m_gpuClockPhase;

	// Divide by the ratio while always rounding up to make sure
	// we're never triggered too early.
	Cycles ratio = gpuToCpuClockRatio().getFp();
	delta = (delta + ratio - 1) / ratio;

	timeKeeper.setNextSyncDelta(Peripheral::PERIPHERAL_GPU, delta);
}

bool Gpu::inVblank() const
{
	return m_displayLine < m_displayLineStart || m_displayLine >= m_displayLineEnd;
}

uint16_t Gpu::displayedVramLine() const
{
	uint16_t offset = m_displayLine;
	if (m_interlaced)
		offset = m_displayLine * 2 + (uint16_t)m_field;

	// The VRAM "wraps around" so in the case of an overflow,
	// we simply truncate to 9 bits
	return (m_displayVramYStart + offset) & 0x1ff;
}

template<typename T>
T Gpu::load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset)
{
	if (!std::is_same<uint32_t, T>::value)
	{
		LOG("Unhandled GPU load");
		return ~0;
	}

	sync(timeKeeper, irqState);

	// GPUSTAT: set bit 26, 27, 28 to signal that the GPU is ready for DMA and CPU access.
	// This way the BIOS won't dead lock waiting for an event that will never come
	if (offset == 0x0)
		return getReadRegister();
	if (offset == 0x4)
		return getStatusRegister();
	
	return 0;
}

template uint32_t Gpu::load<uint32_t>(TimeKeeper&, InterruptState&, uint32_t);
template uint16_t Gpu::load<uint16_t>(TimeKeeper&, InterruptState&, uint32_t);
template uint8_t  Gpu::load<uint8_t> (TimeKeeper&, InterruptState&, uint32_t);

template<typename T>
void Gpu::store(TimeKeeper& timeKeeper, Timers& timers, InterruptState& irqState, uint32_t offset, T value)
{
	if (!std::is_same<uint32_t, T>::value)
	{
		LOG("Unhandled GPU store");
		return;
	}

	sync(timeKeeper, irqState);

	if (offset == 0x0)
		gp0(value);
	else if (offset == 0x4)
		gp1(value, timeKeeper, timers, irqState);
	else
		LOG("GPU write 0x" << std::hex << offset << " 0x" << value);
	return;
}

template void Gpu::store<uint32_t>(TimeKeeper&, Timers&, InterruptState&, uint32_t, uint32_t);
template void Gpu::store<uint16_t>(TimeKeeper&, Timers&, InterruptState&, uint32_t, uint16_t);
template void Gpu::store<uint8_t> (TimeKeeper&, Timers&, InterruptState&, uint32_t, uint8_t );

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
	statusRegister |= ((uint32_t)m_vres) << 19;
	statusRegister |= ((uint32_t)m_vmode) << 20;
	statusRegister |= ((uint32_t)m_displayDepth) << 21;
	statusRegister |= ((uint32_t)m_interlaced) << 22;
	statusRegister |= ((uint32_t)m_displayDisabled) << 23;
	//statusRegister |= ((uint32_t)m_interrupt) << 24;
	statusRegister |= ((uint32_t)m_gp0Interrupt) << 24;

	// For now we pretend that the GPU is always ready:
	// Ready to receive command
	statusRegister |= 1 << 26;
	// Ready to send VRAM to CPU
	statusRegister |= 1 << 27;
	// Ready to receive DMA block
	statusRegister |= 1 << 28;

	statusRegister |= ((uint32_t)m_dmaDirection) << 29;

	// Bit 31 should change depending on currently drawn line ( whether it's even or odd in the vblack apparantly )
	// statusRegister |= 0 << 31;

	// Bit 31 is 1 if the currently displayed VRAM line is odd, 0
	// if it's even or if we're in the vertical blanking.
	if (!inVblank())
		statusRegister |= ((uint32_t)(displayedVramLine() & 1)) << 31;

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
		case 0x02:
			commandParameters.gp0WordsRemaining = 3;
			commandParameters.gp0CommandMethod = &Gpu::gp0FillRect;
			break;
		case 0x28:
			commandParameters.gp0WordsRemaining = 5;
			commandParameters.gp0CommandMethod = &Gpu::gp0QuadMonoOpaque;
			break;
		case 0x2c:
			commandParameters.gp0WordsRemaining = 9;
			commandParameters.gp0CommandMethod = &Gpu::gp0QuadTextureBlendOpaque;
			break;
		case 0x2d:
			commandParameters.gp0WordsRemaining = 9;
			commandParameters.gp0CommandMethod = &Gpu::gp0QuadTextureRawOpaque;
			break;
		case 0x2f:
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
		case 0x65:
			commandParameters.gp0WordsRemaining = 4;
			commandParameters.gp0CommandMethod = &Gpu::gp0RectTextureRawOpaque;
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
			assert(0, "Unhandled GP0 command");
			break;
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

void Gpu::gp1(uint32_t value, TimeKeeper& timeKeeper, Timers& timers, InterruptState& irqState)
{
	uint32_t opcode = (value >> 24) & 0xff;

	switch (opcode)
	{
	case 0x0:
		gp1Reset(timeKeeper, irqState);
		timers.videoTimingsChanged(timeKeeper, irqState, *this);
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
		gp1DisplayVerticalRange(value, timeKeeper, irqState);
		break;
	case 0x08:
		gp1DisplayMode(value, timeKeeper, irqState);
		timers.videoTimingsChanged(timeKeeper, irqState, *this);
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

void Gpu::gp0FillRect()
{
	Position topLeft = Position::fromGP0(m_gp0Command[1]);
	Position size = Position::fromGP0(m_gp0Command[2]);

	Position positions[] = {
		topLeft,
		Position(topLeft.getX() + size.getX(), topLeft.getY()),
		Position(topLeft.getX(), topLeft.getY() + size.getY()),
		Position(topLeft.getX() + size.getX(), topLeft.getY() + size.getY())
	};

	Color colors[] = {
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0])
	};
	m_renderer.pushQuad(positions, colors);
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

void Gpu::gp0QuadTextureRawOpaque()
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

void Gpu::gp0RectTextureRawOpaque()
{
	Position topLeft = Position::fromGP0(m_gp0Command[1]);
	Position size = Position::fromGP0(m_gp0Command[3]);

	Position positions[] = {
		topLeft,
		Position(topLeft.getX() + size.getX(), topLeft.getY()),
		Position(topLeft.getX(), topLeft.getY() + size.getY()),
		Position(topLeft.getX() + size.getX(), topLeft.getY() + size.getY())
	};

	Color colors[] = {
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0]),
		Color::fromGP0(m_gp0Command[0])
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
	//m_renderer.display();
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

void Gpu::gp1Reset(TimeKeeper& timeKeeper, InterruptState& irqState)
{
	//m_interrupt = false;

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
	m_field = Field::FIELD_TOP;

	m_vmode = VMode::VMODE_NTSC;
	m_interlaced = true;
	m_displayHorizStart = 0x200;
	m_displayHorizEnd = 0xc00;
	m_displayLineStart = 0x10;
	m_displayLineEnd = 0x100;
	m_displayDepth = DisplayDepth::DISPLAY_DEPTH_15_BITS;
	m_displayLine = 0;
	m_displayLineTick = 0;

	m_renderer.setDrawOffset(0, 0);

	gp1ResetCommandBuffer();
	gp1AcknowledgeIrq();

	sync(timeKeeper, irqState);
}

void Gpu::gp1ResetCommandBuffer()
{
	m_gp0Command.clear();
	m_gp0WordsRemaining = 0x0;
	m_gp0Mode = Gp0Mode::GP0_MODE_COMMAND;
}

void Gpu::gp1AcknowledgeIrq()
{
	// m_interrupt = false;
	m_gp0Interrupt = false;
}

void Gpu::gp1DisplayEnable(uint32_t value)
{
	m_displayDisabled = value & 1;
}

void Gpu::gp1DisplayMode(uint32_t value, TimeKeeper& timeKeeper, InterruptState& irqState)
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
	m_field = Field::FIELD_TOP;

	if (value & 0x80)
		LOG("Unsupported display mode 0x" << std::hex << val);

	sync(timeKeeper, irqState);
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

void Gpu::gp1DisplayVerticalRange(uint32_t value, TimeKeeper& timeKeeper, InterruptState& irqState)
{
	m_displayLineStart = value & 0x3ff;
	m_displayLineEnd = (value >> 10) & 0x3ff;

	sync(timeKeeper, irqState);
}
