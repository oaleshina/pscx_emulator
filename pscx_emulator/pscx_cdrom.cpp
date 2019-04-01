#include <cassert>

#include "pscx_cdrom.h"
#include "pscx_common.h"
#include "pscx_cpu.h"

// ***************** Fifo implementation *********************
Fifo Fifo::fromBytes(std::vector<uint8_t> bytes)
{
	Fifo fifo;
	for (auto byte : bytes)
		fifo.push(byte);
	return fifo;
}

bool Fifo::isEmpty() const
{
	// If both pointers point at the same cell and have the same carry the
	// FIFO is empty.
	return m_writeIdx == m_readIdx;
}

bool Fifo::isFull() const
{
	// The FIFO is full if both indexes point to the same cell
	// while having a different carry.
	return m_writeIdx == (m_readIdx ^ 0x10);
}

void Fifo::clear()
{
	m_writeIdx = 0x0;
	m_readIdx = 0x0;
	memset(m_buffer, 0x0, sizeof(m_buffer));
}

uint8_t Fifo::len() const
{
	return (m_writeIdx - m_readIdx) & 0x1f;
}

void Fifo::push(uint8_t value)
{
	m_buffer[m_writeIdx & 0xf] = value;
	m_writeIdx = (m_writeIdx + 1) & 0x1f;
}

uint8_t Fifo::pop()
{
	uint8_t idx = m_readIdx & 0xf;
	m_readIdx = (m_readIdx + 1) & 0x1f;
	return m_buffer[idx];
}

// ***************** CdRom implementation *********************
template<typename T>
T CdRom::load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset)
{
	assert((std::is_same<uint8_t, T>::value), "Unhandled CDROM load");

	sync(timeKeeper, irqState);

	uint8_t value = 0x0;
	switch (offset)
	{
	case 0x0:
		value = getStatus();
		break;
	case 0x1:
		if (m_response.isEmpty())
			LOG("CDROM response FIFO underflow");
		
		value = m_response.pop();
		break;
	case 0x3:
		// IRQ mask/flags have the 3 MSB set when read.
		if (m_index == 0x0)
			value = m_irqMask | 0xe0;
		else if (m_index == 0x1)
			value = m_irqFlags | 0xe0;
		break;
	}
	return (uint32_t)value;
}

template uint32_t CdRom::load<uint32_t>(TimeKeeper&, InterruptState&, uint32_t);
template uint16_t CdRom::load<uint16_t>(TimeKeeper&, InterruptState&, uint32_t);
template uint8_t  CdRom::load<uint8_t >(TimeKeeper&, InterruptState&, uint32_t);

template<typename T>
void CdRom::store(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset, T value)
{
	assert((std::is_same<uint8_t, T>::value), "Unhandled CDROM store");

	sync(timeKeeper, irqState);

	// All writeable registers are 8 bit wide
	// bool prevIrq = irq();

	switch (offset)
	{
	case 0x0:
		setIndex((uint8_t)value);
		break;
	case 0x1:
		if (m_index == 0x0)
			command(timeKeeper, (uint8_t)value);
		break;
	case 0x2:
		if (m_index == 0x0)
			pushParam((uint8_t)value);
		else if (m_index == 0x1)
			irqMask((uint8_t)value);
		break;
	case 0x3:
		if (m_index == 0x0)
		{
			setConfig(value);
		}
		else if (m_index == 0x1)
		{
			irqAck((uint8_t)value & 0x1f);
			if (value & 0x40)
				m_params.clear();

			assert((value & 0xa0) == 0x0, "Unhandled CDROM 3.1");
		}
		break;
	}

	//if (!prevIrq && irq())
	//{
		// Interrupt rising edge
	//	irqState.raiseAssert(Interrupt::INTERRUPT_CDROM);
	//}
}

template void CdRom::store<uint32_t>(TimeKeeper&, InterruptState&, uint32_t, uint32_t);
template void CdRom::store<uint16_t>(TimeKeeper&, InterruptState&, uint32_t, uint16_t);
template void CdRom::store<uint8_t >(TimeKeeper&, InterruptState&, uint32_t, uint8_t );

void CdRom::sync(TimeKeeper& timeKeeper, InterruptState& irqState)
{
	Cycles delta = timeKeeper.sync(Peripheral::PERIPHERAL_CDROM);

	CommandState newCommandState;
	switch (m_commandState)
	{
	case CommandState::COMMAND_STATE_IDLE:
	{
		timeKeeper.noSyncNeeded(Peripheral::PERIPHERAL_CDROM);
		newCommandState = CommandState::COMMAND_STATE_IDLE;
		break;
	}
	case CommandState::COMMAND_STATE_RX_PENDING:
	{
		if ((Cycles)m_helperRxPending.m_rxDelay > delta)
		{
			// Update the counters.
			m_helperRxPending.m_rxDelay -= (uint32_t)delta;
			m_helperRxPending.m_irqDelay -= (uint32_t)delta;

			timeKeeper.setNextSyncDelta(Peripheral::PERIPHERAL_CDROM, (Cycles)m_helperRxPending.m_rxDelay);
			newCommandState = CommandState::COMMAND_STATE_RX_PENDING;
		}
		else
		{
			// We reached the end of the transfer.
			m_response = m_helperRxPending.m_response;

			if ((Cycles)m_helperRxPending.m_irqDelay > delta)
			{
				// Schedule the interrupt.
				m_helperRxPending.m_irqDelay -= (uint32_t)delta;
				timeKeeper.setNextSyncDelta(Peripheral::PERIPHERAL_CDROM, (Cycles)m_helperRxPending.m_irqDelay);
				newCommandState = CommandState::COMMAND_STATE_IRQ_PENDING;
			}
			else
			{
				// IRQ is reached.
				triggerIrq(irqState, m_helperRxPending.m_irqCode);
				timeKeeper.noSyncNeeded(Peripheral::PERIPHERAL_CDROM);
				newCommandState = CommandState::COMMAND_STATE_IDLE;
			}
		}
		break;
	}
	case CommandState::COMMAND_STATE_IRQ_PENDING:
	{
		if ((Cycles)m_helperIrqPending.m_irqDelay > delta)
		{
			// The interrupt hasn't been reached yet.
			m_helperIrqPending.m_irqDelay -= (uint32_t)delta;
			timeKeeper.setNextSyncDelta(Peripheral::PERIPHERAL_CDROM, (Cycles)m_helperIrqPending.m_irqDelay);
			newCommandState = CommandState::COMMAND_STATE_IRQ_PENDING;
		}
		else
		{
			// IRQ is reached.
			triggerIrq(irqState, m_helperIrqPending.m_irqCode);
			timeKeeper.noSyncNeeded(Peripheral::PERIPHERAL_CDROM);
			newCommandState = CommandState::COMMAND_STATE_IDLE;
		}
		break;
	}
	}

	m_commandState = newCommandState;

	// See if we have a read pending.
	if (ReadState::READ_STATE_READING == m_readState)
	{
		uint32_t nextSync;
		if ((Cycles)m_helperReading.m_delay > delta)
		{
			// Not yet there.
			nextSync = m_helperReading.m_delay - (uint32_t)delta;
		}
		else
		{
			// A sector has been read from the disc.
			sectorRead(irqState);
			// Prepare for the next one.
			nextSync = getCyclesPerSector();
		}
		m_helperReading.m_delay = nextSync;
		m_readState = ReadState::READ_STATE_READING;
		timeKeeper.setNextSyncDeltaIfCloser(Peripheral::PERIPHERAL_CDROM, (Cycles)nextSync);
	}
}

uint8_t CdRom::readByte()
{
	assert(m_rxIndex < 0x800, "Unhandled CDROM long read");

	uint8_t byte = m_rxSector->getDataByte(m_rxIndex);

	assert(m_rxActive, "Read byte while !m_rxActive");
	m_rxIndex += 1;
	
	return byte;
}

uint32_t CdRom::dmaReadWord()
{
	uint32_t b0 = readByte();
	uint32_t b1 = readByte();
	uint32_t b2 = readByte();
	uint32_t b3 = readByte();

	// Pack in a little endian word.
	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

const Disc* CdRom::getDiscOrDie()
{
	return m_disc;
}

void CdRom::sectorRead(InterruptState& irqState)
{
	LOG("CDROM: read sector at position " << std::hex << m_readPosition.getMinute() << ":" << m_readPosition.getSecond() << ":" << m_readPosition.getFrame());

	XaSector::ResultXaSector resultXaSector = const_cast<Disc*>(getDiscOrDie())->readDataSector(m_readPosition);
	assert(resultXaSector.getSectorStatus() == XaSector::XaSectorStatus::XA_SECTOR_STATUS_OK, "Couldn't read sector");
	m_rxSector = resultXaSector.getSectorPtr();

	if (m_irqFlags == 0x0)
	{
		// TODO: wrap getDriveStatus() to return Fifo object
		m_response = Fifo::fromBytes({ getDriveStatus() });

		// Trigger interrupt.
		triggerIrq(irqState, IrqCode::IRQ_CODE_SECTOR_READY);
	}

	// Move on to the next segment.
	m_readPosition = m_readPosition.getNextSector();
}

uint8_t CdRom::getStatus()
{
	uint8_t status = m_index;

	status |= 0 << 2;
	status |= ((uint8_t)m_params.isEmpty()) << 3;
	status |= ((uint8_t)(!m_params.isFull())) << 4;
	status |= ((uint8_t)(!m_params.isEmpty())) << 5;
	status |= 0 << 6;
	// "Busy" flag
	if (m_commandState == CommandState::COMMAND_STATE_RX_PENDING)
		status |= 1 << 7;

	return status;
}

bool CdRom::irq() const
{
	return m_irqFlags & m_irqMask;
}

void CdRom::triggerIrq(InterruptState& irqState, IrqCode irq)
{
	assert(m_irqFlags == 0x0, "Unsupported nested CDROM interrupt");

	bool prevIrq = this->irq();
	m_irqFlags = (uint8_t)irq;

	if (!prevIrq && this->irq())
	{
		// Interrupt rising edge.
		irqState.raiseAssert(Interrupt::INTERRUPT_CDROM);
	}
}

void CdRom::setIndex(uint8_t index)
{
	m_index = index & 3;
}

void CdRom::irqAck(uint8_t value)
{
	m_irqFlags &= (~value);

	if (m_irqFlags == 0)
	{
		assert(m_commandState == CommandState::COMMAND_STATE_IDLE, "CDROM IRQ acknowledge while controller is busy");
		
		// Certain commands have a 2nd phase after the first interrupt is acknowledged.
		auto onAcknowledgeFuncPtr = m_onAcknowledge;
		m_onAcknowledge = &CdRom::ackIdle;
		m_commandState = (this->*onAcknowledgeFuncPtr)();
	}
}

void CdRom::setConfig(uint8_t config)
{
	bool prevActive = m_rxActive;
	m_rxActive = config & 0x80;

	if (m_rxActive)
	{
		if (!prevActive)
		{
			// Reset the index to the beginning of the RX buffer.
			m_rxIndex = 0x0;
		}
	}
	else
	{
		// Align to the next multiple of 8 bytes.
		m_rxIndex = (m_rxIndex & ~7) + ((m_rxIndex & 4) << 1);
	}

	assert((config & 0x7f) == 0, "CDROM: unhandled config");
}

void CdRom::irqMask(uint8_t value)
{
	m_irqMask = value & 0x1f;
}

uint32_t CdRom::getCyclesPerSector()
{
	// 1x speed: 75 sectors per second
	return (CPU_FREQ_HZ / 75) >> m_doubleSpeed;
}

void CdRom::command(TimeKeeper& timeKeeper, uint8_t cmd)
{
	assert(m_commandState == CommandState::COMMAND_STATE_IDLE, "CDROM command while controller is busy");

	m_response.clear();

	CommandState(CdRom::*onAcknowledge)(void) = nullptr;
	switch (cmd)
	{
	case 0x01:
		onAcknowledge = &CdRom::cmdGetStat;
		break;
	case 0x02:
		onAcknowledge = &CdRom::cmdSetLoc;
		break;
	case 0x06:
		onAcknowledge = &CdRom::cmdReadN;
		break;
	case 0x09:
		onAcknowledge = &CdRom::cmdPause;
		break;
	case 0x0e:
		onAcknowledge = &CdRom::cmdSetMode;
		break;
	case 0x15:
		onAcknowledge = &CdRom::cmdSeekl;
		break;
	case 0x1a:
		onAcknowledge = &CdRom::cmdGetId;
		break;
	case 0x1e:
		onAcknowledge = &CdRom::cmdReadToc;
		break;
	case 0x19:
		onAcknowledge = &CdRom::cmdTest;
		break;
	default:
		assert(0, "Unhandled CDROM command");
		break;
	}

	if (m_irqFlags == 0)
	{
		// If the previous command ( if any ) has been acknowledged
		// we can directly start the new one.
		m_commandState = (this->*onAcknowledge)();

		// Schedule the interrupt if needed.
		if (CommandState::COMMAND_STATE_RX_PENDING == m_commandState)
		{
			timeKeeper.setNextSyncDelta(Peripheral::PERIPHERAL_CDROM, m_helperRxPending.m_irqDelay);
		}
	}
	else
	{
		// Schedule the command to be executed once the current one is acknowledged.
		m_onAcknowledge = onAcknowledge;
	}

	if (m_readState == ReadState::READ_STATE_READING)
	{
		timeKeeper.setNextSyncDeltaIfCloser(Peripheral::PERIPHERAL_CDROM, m_helperReading.m_delay);
	}

	m_params.clear();
}

uint8_t CdRom::getDriveStatus() const
{
	if (m_disc)
	{
		bool reading = !(m_readState == ReadState::READ_STATE_IDLE);

		uint8_t driveStatus = 0x0;
		// Motor on.
		driveStatus |= 1 << 1;
		driveStatus |= ((uint8_t)reading) << 5;
		return driveStatus;
	}
	// No disc, pretemd that the shell is open ( bit 4 ).
	return 0x10;
}

CommandState CdRom::cmdGetStat()
{
	// If this occurs on the real hardware it should set bit 1 of the result byte
	// and then put a 2nd byte 0x20 to signal the wrong number of params.
	assert(m_params.isEmpty(), "Unexpected parmeters for CDROM GetStat command");

	// m_response.push(0x10);
	// triggerIrq(IrqCode::IRQ_CODE_OK);
	Fifo response;
	response.push(getDriveStatus());

	// The response comes earlier when there's no disc.
	uint32_t rxDelay = 0x0;
	if (m_disc)
	{
		// Average measured delay with the game disc.
		rxDelay = 24'000;
	}
	else
	{
		// Average measured delay with the shell opened.
		rxDelay = 17'000;
	}

	m_helperRxPending.m_rxDelay = rxDelay;
	m_helperRxPending.m_irqDelay = rxDelay + 5401;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
	m_helperRxPending.m_response = response;
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::cmdSetLoc()
{
	assert(m_params.len() == 3, "CDROM: bad number of parameters for SetLoc");

	// Parameters are in the BCD.
	uint8_t minute = m_params.pop();
	uint8_t second = m_params.pop();
	uint8_t frame  = m_params.pop();

	m_seekTarget = MinuteSecondFrame::fromBCD(minute, second, frame);

	if (m_disc)
	{
		m_helperRxPending.m_rxDelay = 35'000;
		m_helperRxPending.m_irqDelay = 35'000 + 5399;
		m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
		// TODO: byte representation
		m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	}
	else
	{
		m_helperRxPending.m_rxDelay = 25'000;
		m_helperRxPending.m_irqDelay = 25'000 + 6763;
		m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_ERROR;
		// TODO: byte representation
		m_helperRxPending.m_response = Fifo::fromBytes({ 0x11, 0x80 });
	}
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::cmdReadN()
{
	assert(m_readState == ReadState::READ_STATE_IDLE, "CDROM read command while we're already reading");
	m_helperReading.m_delay = getCyclesPerSector();
	m_readState = ReadState::READ_STATE_READING;

	m_helperRxPending.m_rxDelay = 28'000;
	m_helperRxPending.m_irqDelay = 28'000 + 5401;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
	// TODO: byte representation
	m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::cmdPause()
{
	assert(m_readState != ReadState::READ_STATE_IDLE, "Pause when we're not reading");

	m_onAcknowledge = &CdRom::ackPause;

	m_helperRxPending.m_rxDelay = 25'000;
	m_helperRxPending.m_irqDelay = 25'000 + 5393;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
	// TODO: byte representation
	m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::cmdSetMode()
{
	assert(m_params.len() == 1, "CDROM: bad number of parameters for SetMode");

	uint8_t mode = m_params.pop();
	m_doubleSpeed = mode & 0x80;

	assert((mode & 0x7f) == 0x0, "CDROM: unhandled mode");

	m_helperRxPending.m_rxDelay = 22'000;
	m_helperRxPending.m_irqDelay = 22'000 + 5391;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
	// TODO: byte representation
	m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::cmdSeekl()
{
	// Make sure that we don't end up in the track 1's pregap.
	// TODO: implement operator>
	assert(m_seekTarget > MinuteSecondFrame::fromBCD(0x0, 0x2, 0x0), "Seek to track 1 pregap");

	m_readPosition = m_seekTarget;
	m_onAcknowledge = &CdRom::ackSeekl;

	m_helperRxPending.m_rxDelay = 35'000;
	m_helperRxPending.m_irqDelay = 35'000 + 5401;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
	// TODO: byte representation
	m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::cmdGetId()
{
	if (m_disc)
	{
		// When a disc is present we have two responses: first
		// we answer with the status byte and when it's acknowledged
		// we send the actual disc identification sequence.
		m_onAcknowledge = &CdRom::ackGetId;

		// First response: status byte
		m_helperRxPending.m_rxDelay = 26'000;
		m_helperRxPending.m_irqDelay = 26'000 + 5401;
		m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
		// TODO: byte representation
		m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	}
	else
	{
		// Pretend the shell is open.
		m_helperRxPending.m_rxDelay = 20'000;
		m_helperRxPending.m_irqDelay = 20'000 + 6776;
		m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_ERROR;
		// TODO: byte representation
		m_helperRxPending.m_response = Fifo::fromBytes({ 0x11, 0x80 });
	}
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::cmdReadToc()
{
	m_onAcknowledge = &CdRom::ackReadToc;

	m_helperRxPending.m_rxDelay = 45'000;
	m_helperRxPending.m_irqDelay = 45'000 + 5401;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
	// TODO: byte representation
	m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::cmdTest()
{
	assert(m_params.len() == 0x1, "Unexpected number of parameters for CDROM test command");

	switch (m_params.pop())
	{
	case 0x20:
		return testVersion();
	default:
		assert(0, "Unhandled CDROM test subcommand");
	}
}

CommandState CdRom::testVersion()
{
	// Values returned by PAL SCPH-7502:
	Fifo response = Fifo::fromBytes({ /*Year=*/0x98, /*Month=*/0x06, /*Day=*/0x10, /*Version=*/0xc3 });

	uint32_t rxDelay = 0x0;
	if (m_disc)
	{
		// Average measured delay with the game disc
		rxDelay = 21'000;
	}
	else
	{
		// Average measured delay with the shell opened.
		rxDelay = 29'000;
	}

	m_helperRxPending.m_rxDelay = rxDelay;
	m_helperRxPending.m_irqDelay = rxDelay + 9711;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_OK;
	m_helperRxPending.m_response = response;
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::ackIdle()
{
	return CommandState::COMMAND_STATE_IDLE;
}

CommandState CdRom::ackSeekl()
{
	// The seek itself takes a while to finish since the drive has
	// to physically move the head.
	m_helperRxPending.m_rxDelay = 1'000'000;
	m_helperRxPending.m_irqDelay = 1'000'000 + 1859;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_DONE;
	// TODO: byte representation
	m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::ackGetId()
{
	if (m_disc)
	{
		uint8_t regionSymbol;
		switch (m_disc->getRegion())
		{
		case Region::REGION_JAPAN:
			regionSymbol = 'I';
			break;
		case Region::REGION_NORTH_AMERICA:
			regionSymbol = 'A';
			break;
		case Region::REGION_EUROPE:
			regionSymbol = 'E';
			break;
		}

		Fifo response = Fifo::fromBytes({
			// Status + bit 3 if unlicensed/audio.
			getDriveStatus(),
			// Licensed, not audio, not missing.
			0x0,
			// Disc type.
			0x20,
			// 8 bit ATIP from Point = C0h, if session info exists.
			0x0,
			// Region string: "SCEI" for Japan, "SCEE" for Europe
			// and "SCEA" for US.
			'S', 'C', 'E',
			regionSymbol
			});

		m_helperRxPending.m_rxDelay = 7'336;
		m_helperRxPending.m_irqDelay = 7'336 + 12'376;
		m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_DONE;
		m_helperRxPending.m_response = response;
		return CommandState::COMMAND_STATE_RX_PENDING;
	}
	else
	{
		assert(0, "ackGetId: Unreachable!");
	}
}

CommandState CdRom::ackReadToc()
{
	uint32_t rxDelay = 11'000;
	if (m_disc)
		rxDelay = 16'000'000;
	
	m_helperRxPending.m_rxDelay = rxDelay;
	m_helperRxPending.m_irqDelay = rxDelay + 1859;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_DONE;
	// TODO: byte representation
	m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	return CommandState::COMMAND_STATE_RX_PENDING;
}

CommandState CdRom::ackPause()
{
	m_readState = ReadState::READ_STATE_IDLE;

	m_helperRxPending.m_rxDelay = 2'000'000;
	m_helperRxPending.m_irqDelay = 2'000'000 + 1858;
	m_helperRxPending.m_irqCode = IrqCode::IRQ_CODE_DONE;
	// TODO: byte representation
	m_helperRxPending.m_response = Fifo::fromBytes({ getDriveStatus() });
	return CommandState::COMMAND_STATE_RX_PENDING;
}

void CdRom::pushParam(uint8_t param)
{
	if (m_params.isFull())
		LOG("CDROM parameter FIFO overflow");
	m_params.push(param);
}
