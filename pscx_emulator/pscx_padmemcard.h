#pragma once

// Gamepad and memory card controller emulation

#include <cstdint>

#include "pscx_gamepad.h"
#include "pscx_timekeeper.h"
#include "pscx_interrupts.h"

// Identifies the tagret of the serial communication, either the gamepad/memory card port 0 or 1.
enum class Target
{
	TARGET_PAD_MEMCARD1 = 0,
	TARGET_PAD_MEMCARD2 = 1
};

// Controller transaction state machine.
enum class BusState
{
	// Bus is idle.
	BUS_STATE_IDLE,

	// Transaction in progress, we store the response byte, the DSR
	// response and the number of Cycles remaining until we reach the DSR pulse (if any).
	BUS_STATE_TRANSFER,

	// DSR is asserted, count the number of cycles remaining.
	BUS_STATE_DSR
};

struct PadMemCard
{
	PadMemCard();

	template<typename T>
	void store(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset, T value);

	template<typename T>
	T load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset);

	void sync(TimeKeeper& timeKeeper, InterruptState& irqState);

	// Return a mutable reference to the gamepad profiles being used.
	std::vector<Profile*> getPadProfiles() const;

	void sendCommand(TimeKeeper& timeKeeper, uint8_t cmd);

	uint32_t getStat() const;

	void setMode(uint8_t mode);

	uint16_t getControl() const;
	void setControl(InterruptState& irqState, uint16_t ctrl);

private:
	// Serial clock divider. The LSB is read/write but is not used.
	// This way the hardware divide the CPU clock by half of "baud rate divider"
	// and can invert the serial clock polarity twice every "baud rate divider"
	// which effectively means that the resulting frequency is CPU clock / ( "baud rate divider" & 0xfe ).
	uint16_t m_baudRateDivider;

	// Serial config.
	uint8_t m_mode;

	// Transmission enabled if true.
	bool m_transmissionEnabled;

	// If true the targeted peripheral select signal is asserted
	// ( the actual signal is active low, so it's driving low on the controller
	// port when "select" is true ). The "target" field says which peripheral is addressed.
	bool m_select;

	// This bit says which of the two pad/memorycard port pair we're selecting with "select" above.
	// Multitaps are handled at the serial protocol level, not by dedicated hardware pins.
	Target m_target;

	// Data Set Ready signal, active low ( driven by the gamepad ).
	bool m_dataSetReadySignal;

	// If true an interrupt is generated when a DSR pulse is received from the pad/memory card.
	bool m_dsrInterrupt;

	// Current interrupt level.
	bool m_interruptLevel;

	// Current response byte.
	uint8_t m_response;

	// True when the RX FIFO is not empty.
	bool m_rxNotEmpty;

	// Gamepad in slot 1.
	GamePad m_pad1;

	// Gamepad in slot 2.
	GamePad m_pad2;

	// Bus state machine.
	BusState m_busState;

	struct BusTransfer
	{
		BusTransfer() :
			m_responseByte(0x0),
			m_dsrResponse(0x0),
			m_cyclesRemaining(0x0)
		{}
		uint8_t m_responseByte;
		bool m_dsrResponse;
		Cycles m_cyclesRemaining;
	} m_helperBusTransfer;

	struct BusDsr
	{
		BusDsr() :
			m_cyclesRemaining(0x0)
		{}
		Cycles m_cyclesRemaining;
	} m_helperBusDsr;
};
