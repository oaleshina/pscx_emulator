#include <cassert>
#include <vector>
#include <algorithm>

#include "pscx_padmemcard.h"
#include "pscx_common.h"

// ********************** PadMemCard implementation **********************
PadMemCard::PadMemCard() :
	m_baudRateDivider(0x0),
	m_mode(0x0),
	m_transmissionEnabled(false),
	m_select(false),
	m_target(Target::TARGET_PAD_MEMCARD1),
	m_interruptLevel(false),
	m_dataSetReadySignal(false),
	m_dsrInterrupt(false),
	m_response(0xff),
	m_rxNotEmpty(false),
	m_pad1(Type::TYPE_DIGITAL),
	m_pad2(Type::TYPE_DISCONNECTED),
	m_busState(BusState::BUS_STATE_IDLE)
{
}

template<typename T>
void PadMemCard::store(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset, T value)
{
	sync(timeKeeper, irqState);

	switch (offset)
	{
	case 0:
		assert((std::is_same<T, uint8_t>::value), "Unhandled gamepad TX access");
		sendCommand(timeKeeper, (uint8_t)value);
		break;
	case 8:
		setMode((uint8_t)value);
		break;
	case 10:
		// Byte access behaves like a halfword
		assert((!std::is_same<T, uint8_t>::value), "Unhandled byte gamepad control access");
		setControl(irqState, (uint16_t)value);
		break;
	case 14:
		m_baudRateDivider = (uint16_t)value;
		break;
	default:
		assert(0, "Unhandled write to gamepad register");
	}
}

template void PadMemCard::store<uint32_t>(TimeKeeper&, InterruptState&, uint32_t, uint32_t);
template void PadMemCard::store<uint16_t>(TimeKeeper&, InterruptState&, uint32_t, uint16_t);
template void PadMemCard::store<uint8_t >(TimeKeeper&, InterruptState&, uint32_t, uint8_t);

template<typename T>
T PadMemCard::load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset)
{
	sync(timeKeeper, irqState);

	switch (offset)
	{
	case 0:
	{
		assert((std::is_same<T, uint8_t>::value), "Unhandled gamepad TX access");
		uint32_t response = (uint32_t)m_response;
		m_rxNotEmpty = false;
		m_response = 0xff;
		return response;
	}
	case 4:
	{
		return getStat();
	}
	case 10:
	{
		return (uint32_t)getControl();
	}
	default:
		assert(0, "Unhandled gamepad read");
	}
	return ~0;
}

template uint32_t PadMemCard::load<uint32_t>(TimeKeeper&, InterruptState&, uint32_t);
template uint16_t PadMemCard::load<uint16_t>(TimeKeeper&, InterruptState&, uint32_t);
template uint8_t  PadMemCard::load<uint8_t >(TimeKeeper&, InterruptState&, uint32_t);

void PadMemCard::sync(TimeKeeper& timeKeeper, InterruptState& irqState)
{
	Cycles delta = timeKeeper.sync(Peripheral::PERIPHERAL_GPU);

	switch (m_busState)
	{
	case BusState::BUS_STATE_IDLE:
		timeKeeper.noSyncNeeded(Peripheral::PERIPHERAL_PAD_MEMCARD);
		break;
	case BusState::BUS_STATE_TRANSFER:
		if (delta < m_helperBusTransfer.m_cyclesRemaining)
		{
			m_helperBusTransfer.m_cyclesRemaining -= delta;
			if (m_dsrInterrupt)
				timeKeeper.setNextSyncDelta(Peripheral::PERIPHERAL_PAD_MEMCARD, m_helperBusTransfer.m_cyclesRemaining);
			else
				timeKeeper.noSyncNeeded(Peripheral::PERIPHERAL_PAD_MEMCARD);
		}
		else
		{
			// We reached the end of the transfer
			assert(!m_rxNotEmpty, "Gamepad RX while FIFO isn't empty");

			m_response = m_helperBusTransfer.m_responseByte;
			m_rxNotEmpty = true;
			m_dataSetReadySignal = m_helperBusTransfer.m_dsrResponse;

			if (m_dataSetReadySignal)
			{
				if (m_dsrInterrupt)
				{
					if (!m_interruptLevel)
					{
						// Rising edge of the interrupt
						irqState.raiseAssert(Interrupt::INTERRUPT_PAD_MEMCARD);
					}
					m_interruptLevel = true;
				}

				// The DSR pulse is generated purely by the controller
				// without any input from the console. Therefore the actual length of the pulse
				// changes from controller to controller.
				m_helperBusDsr.m_cyclesRemaining = 10;
				m_busState = BusState::BUS_STATE_DSR;
			}
			else
			{
				// We're done with this transaction
				m_busState = BusState::BUS_STATE_IDLE;
			}

			timeKeeper.noSyncNeeded(Peripheral::PERIPHERAL_PAD_MEMCARD);
		}
		break;
	case BusState::BUS_STATE_DSR:
		if (delta < m_helperBusDsr.m_cyclesRemaining)
		{
			m_helperBusDsr.m_cyclesRemaining -= delta;
		}
		else
		{
			// DSR pulse is over, bus is idle
			m_dataSetReadySignal = false;
			m_busState = BusState::BUS_STATE_IDLE;
		}
		timeKeeper.noSyncNeeded(Peripheral::PERIPHERAL_PAD_MEMCARD);
		break;
	}
}

std::vector<Profile*> PadMemCard::getPadProfiles()
{
	std::vector<Profile*> padProfiles{ &m_pad1.getProfile(), &m_pad2.getProfile() };
	return padProfiles;
}

void PadMemCard::sendCommand(TimeKeeper& timeKeeper, uint8_t cmd)
{
	// It should be stored in the FIFO and sent when transmissionEnabled is set.
	assert(m_transmissionEnabled, "Unhandled gamepad command while transmissionEnabled is disabled");

	if (m_busState != BusState::BUS_STATE_IDLE)
		LOG("Gamepad command 0x" << std::hex << cmd << "while bus is busy!");

	std::pair<uint8_t, bool> command;
	if (m_select)
	{
		if (m_target == Target::TARGET_PAD_MEMCARD1)
			command = m_pad1.sendCommand(cmd);
		else if (m_target == Target::TARGET_PAD_MEMCARD2)
			command = m_pad2.sendCommand(cmd);
	}
	else
	{
		// No response.
		command = std::make_pair(0xff, false);
	}

	// We're sending 8 bits, one every "baud rate divider" CPU cycles.
	Cycles transmissionDuration = 8 * m_baudRateDivider;

	m_helperBusTransfer.m_responseByte = command.first;
	m_helperBusTransfer.m_dsrResponse = command.second;
	m_helperBusTransfer.m_cyclesRemaining = transmissionDuration;
	m_busState = BusState::BUS_STATE_TRANSFER;

	if (m_dsrInterrupt)
	{
		// The DSR pulse follows immediately after the last byte.
		timeKeeper.setNextSyncDelta(Peripheral::PERIPHERAL_PAD_MEMCARD, transmissionDuration);
	}
}

uint32_t PadMemCard::getStat() const
{
	uint32_t stat(0x0);

	// TX ready bits 1 and 2.
	stat |= 5;
	stat |= (uint32_t)m_rxNotEmpty << 1;
	// RX parity error should always be 0.
	stat |= 0 << 3;
	// Pretend the ack line ( active low ) is always high.
	stat |= 0 << 7;
	stat |= (uint32_t)m_interruptLevel << 9;
	stat |= 0 << 11;

	return stat;
}

void PadMemCard::setMode(uint8_t mode)
{
	m_mode = mode;
}

uint16_t PadMemCard::getControl() const
{
	uint16_t ctrl(0x0);

	ctrl |= (uint16_t)m_transmissionEnabled;
	ctrl |= (uint16_t)m_select << 1;
	ctrl |= (uint16_t)m_dsrInterrupt << 12;
	ctrl |= (uint16_t)m_target << 13;

	return ctrl;
}

void PadMemCard::setControl(InterruptState& irqState, uint16_t ctrl)
{
	if (ctrl & 0x40)
	{
		// Soft reset.
		m_baudRateDivider = 0x0;
		m_mode = 0x0;
		m_select = false;
		m_target = Target::TARGET_PAD_MEMCARD1;
		m_interruptLevel = false;
		m_rxNotEmpty = false;
		m_busState = BusState::BUS_STATE_IDLE;
		m_dataSetReadySignal = false;
	}
	else
	{
		if (ctrl & 0x10)
		{
			// Interrupt acknowledge
			m_interruptLevel = false;

			if (m_dataSetReadySignal && m_dsrInterrupt)
			{
				// The controllers's "dsrInterrupt" interrupt is not edge
				// triggered: as long as m_dataSetReadySignal && m_dsrInterrupt is true
				// it will keep being triggered. If the software attempts to acknowledge
				// the interrupt in this state, it will re-trigger immediately which will be seen
				// by the edge-triggered top level interrupt controller.
				LOG("Gamepad interrupt acknowledge while DSR is active");

				m_interruptLevel = true;
				irqState.raiseAssert(Interrupt::INTERRUPT_PAD_MEMCARD);
			}
		}

		bool previousSelect = m_select;

		//m_unknown = ((uint8_t)ctrl) & 0x28;
		m_transmissionEnabled = ctrl & 1;
		m_select = (ctrl >> 1) & 1;
		//m_rxEnabled = (ctrl >> 2) & 1;
		m_dsrInterrupt = (ctrl >> 12) & 1;
		if (ctrl & 0x2000 == 0) m_target = Target::TARGET_PAD_MEMCARD1;
		else m_target = Target::TARGET_PAD_MEMCARD2;

		//assert(!m_rxEnabled, "Gamepad rxEnabled is not implemented");

		assert(!(m_dsrInterrupt && !m_interruptLevel && m_dataSetReadySignal), "dsrInterrupt is enabled while DSR signal is active");
		assert(!(ctrl & 0xf00), "Unsupported gamepad interrupts");

		if (!previousSelect && m_select)
			m_pad1.setSelect();
	}
}
