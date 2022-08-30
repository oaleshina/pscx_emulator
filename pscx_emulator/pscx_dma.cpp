#include "pscx_dma.h"

uint32_t Dma::getDmaControlRegister() const
{
	return m_control;
}

void Dma::setDmaControlRegister(uint32_t value)
{
	m_control = value;
}

bool Dma::getIRQStatus() const
{
	uint8_t channelIRQ = m_channelIRQFlags & m_channelIRQEnabled;
	return m_forceIRQ || (m_masterIRQEnabled && channelIRQ);
}

uint32_t Dma::getDmaInterruptRegister() const
{
	uint32_t interruptRegister = 0;

	interruptRegister |= (uint32_t)m_IRQDummy;
	interruptRegister |= ((uint32_t)m_forceIRQ) << 15;
	interruptRegister |= ((uint32_t)m_channelIRQEnabled) << 16;
	interruptRegister |= ((uint32_t)m_masterIRQEnabled) << 23;
	interruptRegister |= ((uint32_t)m_channelIRQFlags) << 24;
	interruptRegister |= ((uint32_t)getIRQStatus()) << 31;

	return interruptRegister;
}

void Dma::setDmaInterruptRegister(uint32_t value, InterruptState& irqState)
{
	uint32_t prevIrq = getDmaInterruptRegister();

	m_IRQDummy = value & 0x3f;
	m_forceIRQ = (value >> 15) & 1;
	m_channelIRQEnabled = (value >> 16) & 0x7f;
	m_masterIRQEnabled = (value >> 23) & 1;

	uint8_t ack = (value >> 24) & 0x3f;
	m_channelIRQFlags &= ~ack;

	if (!prevIrq && getDmaInterruptRegister())
	{
		// Rising edge of the done interrupt
		irqState.raiseAssert(Interrupt::INTERRUPT_DMA);
	}
}

const Channel& Dma::getDmaChannelRegister(Port port) const
{
	return m_channels[port];
}

Channel& Dma::getDmaChannelRegisterMutable(Port port)
{
	return m_channels[port];
}

void Dma::done(Port port, InterruptState& irqState)
{
	getDmaChannelRegisterMutable(port).done();

	uint32_t prevIrq = getDmaInterruptRegister();

	// Set interrupt flag if the channel's interrupt is enabled
	uint8_t interruptEnabled = m_channelIRQEnabled & (1 << port);
	m_channelIRQFlags |= interruptEnabled;

	if (!prevIrq && getDmaInterruptRegister())
	{
		// Rising edge of the done interrupt
		irqState.raiseAssert(Interrupt::INTERRUPT_DMA);
	}
}

uint32_t Channel::getDmaChannelControlRegister() const
{
	uint32_t channelControlRegister = 0;

	channelControlRegister |= static_cast<uint32_t>(m_direction);
	channelControlRegister |= static_cast<uint32_t>(m_step) << 1;
	channelControlRegister |= static_cast<uint32_t>(m_chop) << 8;
	channelControlRegister |= static_cast<uint32_t>(m_sync) << 9;
	channelControlRegister |= static_cast<uint32_t>(m_chopDmaWindowSize) << 16;
	channelControlRegister |= static_cast<uint32_t>(m_chopCpuWindowSize) << 20;
	channelControlRegister |= static_cast<uint32_t>(m_enable) << 24;
	channelControlRegister |= static_cast<uint32_t>(m_trigger) << 28;
	channelControlRegister |= static_cast<uint32_t>(m_dummy) << 29;

	return channelControlRegister;
}

void Channel::setDmaChannelControlRegister(uint32_t value)
{
	m_direction = (value & 1) ? Direction::DIRECTION_FROM_RAM : Direction::DIRECTION_TO_RAM;
	m_step = (value >> 1) & 1 ? Step::STEP_DECREMENT : Step::STEP_INCREMENT;
	m_chop = (value >> 8) & 1;

	uint32_t dmaSyncMode = (value >> 9) & 3;
	switch (dmaSyncMode)
	{
	case 0:
		m_sync = Sync::SYNC_MANUAL;
		break;
	case 1:
		m_sync = Sync::SYNC_REQUEST;
		break;
	case 2:
		m_sync = Sync::SYNC_LINKED_LIST;
		break;
	default:
		LOG("Unknown DMA sync mode 0x" << std::hex << dmaSyncMode);
		return;
	}

	m_chopDmaWindowSize = (value >> 16) & 7;
	m_chopCpuWindowSize = (value >> 20) & 7;

	m_enable = (value >> 24) & 1;
	m_trigger = (value >> 28) & 1;
	m_dummy = (value >> 29) & 3;
}

uint32_t Channel::getBaseAddress() const
{
	return m_base;
}

void Channel::setBaseAddress(uint32_t value)
{
	m_base = value & 0xffffff;
}

uint32_t Channel::getBlockControlRegister() const
{
	uint32_t blockSize = m_blockSize;
	uint32_t blockCount = m_blockCount;

	return (blockCount << 16) | blockSize;
}

void Channel::setBlockControlRegister(uint32_t value)
{
	m_blockSize = value;
	m_blockCount = value >> 16;
}

bool Channel::isActiveChannel()
{
	bool trigger = true;
	if (m_sync == Sync::SYNC_MANUAL)
		 trigger = m_trigger;
	return m_enable && trigger;
}

Direction Channel::getDirection() const
{
	return m_direction;
}

Step Channel::getStep() const
{
	return m_step;
}

Sync Channel::getSync() const
{
	return m_sync;
}

uint32_t Channel::getTransferSize()
{
	uint32_t blockSize = m_blockSize;
	uint32_t blockCount = m_blockCount;

	// For manual mode only the block size is used
	if (m_sync == Sync::SYNC_MANUAL)
		return blockSize;
	// In DMA request mode we must transfer "blockCount" blocks
	if (m_sync == Sync::SYNC_REQUEST)
		return blockCount * blockSize;
	// In Linked List mode the size is not known ahead of time: we stop when we encounter
	// the "end of list" marker ( 0xffffff )
	//if (m_sync == Sync::SYNC_LINKED_LIST)
	//	return <undefined>;
	// Return zero for now, implement this function by using std::optional
	return 0x0;
}

void Channel::done()
{
	m_enable = false;
	m_trigger = false;
}
