#pragma once

#include "pscx_common.h"
#include "pscx_memory.h"
#include "pscx_interrupts.h"

// DMA transfer direction
enum Direction
{
	DIRECTION_TO_RAM,
	DIRECTION_FROM_RAM
};

// DMA transfer step
enum Step
{
	STEP_INCREMENT,
	STEP_DECREMENT
};

// DMA transfer synchronization mode
enum Sync
{
	// Transfer starts when the CPU writes to the Trigger bit and transfers everything at once
	SYNC_MANUAL,

	// Sync blocks to DMA requests
	SYNC_REQUEST,

	// Used to transfer GPU command lists
	SYNC_LINKED_LIST
};

// The 7 DMA ports
enum Port
{
	// Macroblock decoder input
	PORT_MDEC_IN,

	// Macroblock decoder output
	PORT_MDEC_OUT,

	// Graphics Processing Unit
	PORT_GPU,

	// CD-ROM drive
	PORT_CD_ROM,

	// Sound Processing Unit
	PORT_SPU,

	// Extension port
	PORT_PIO,

	// Used to clear the ordering table
	PORT_OTC
};

// Per-channel data
struct Channel
{
	Channel() :
		m_enable(false),
		m_direction(Direction::DIRECTION_TO_RAM),
		m_step(Step::STEP_INCREMENT),
		m_sync(Sync::SYNC_MANUAL),
		m_trigger(false),
		m_chop(false),
		m_chopDmaWindowSize(0x0),
		m_chopCpuWindowSize(0x0),
		m_dummy(0x0),
		m_base(0x0),
		m_blockSize(0x0),
		m_blockCount(0x0)
	{}

	uint32_t getDmaChannelControlRegister() const;
	void setDmaChannelControlRegister(uint32_t value);

	uint32_t getBaseAddress() const;

	// Set channel base address. Only bits [0:23] are significant so
	// only 16 MB are addressable by the DMA
	void setBaseAddress(uint32_t value);

	uint32_t getBlockControlRegister() const; // Retrieve value of the Block Control register
	void setBlockControlRegister(uint32_t value); // Set value of the Block Control register

	bool isActiveChannel();

	Direction getDirection() const;
	Step getStep() const;
	Sync getSync() const;

	// Return the DMA transfer size in bytes 
	// Should be handled for linked list mode
	uint32_t getTransferSize();

	// Set the channel status to the "completed" state
	void done();

private:
	bool m_enable; // Enables the channel and starts the transfer in Request or Linked List sync modes
	Direction m_direction; // Transfer direction: RAM-to-device( 0 ) or device-to-RAM( 1 )
	Step m_step; // DMA increment or decrement the address in RAM during the transfer
	Sync m_sync; // Type of synchronization: Manual( 0 ), Request( 1 ) or Linked List( 2 )
	bool m_trigger; // Used to start the DMA transfer when 'sync' is 'Manual'
	bool m_chop; // If true the DMA "chops" the transfer and lets the CPU run in the gaps
	uint8_t m_chopDmaWindowSize; // Chopping DMA window size ( log2 number of words )
	uint8_t m_chopCpuWindowSize; // Chopping CPU window size ( log2 number of cycles )
	uint8_t m_dummy; // Unknown 2 RW bits in configuration register

	uint32_t m_base; // DMA start address

	uint16_t m_blockSize; // Size of a block in words
	uint16_t m_blockCount; // Block count, only used when sync mode is Request
};

// Direct Memory Access
struct Dma
{
	Dma() : 
		m_control(0x07654321) // Reset value 
	{}

	uint32_t getDmaControlRegister() const; // Retrieve the value of the control register
	void setDmaControlRegister(uint32_t value); // Set the value of the control register

	uint32_t getDmaInterruptRegister() const; // Retrieve the value of the interrupt register
	void setDmaInterruptRegister(uint32_t value, InterruptState& irqState); // Set the value of the interrupt register

	const Channel& getDmaChannelRegister(Port port) const; // Return a reference to a channel by port number
	Channel& getDmaChannelRegisterMutable(Port port); // Return a mutable reference to a channel by port number

	void done(Port port, InterruptState& irqState);

	bool getIRQStatus() const; // Return the status of the DMA interrupt

	// Registers
	uint32_t m_control; // DMA control register

	bool m_masterIRQEnabled; // Master IRQ enable
	uint8_t m_channelIRQEnabled; // IRQ enable for individual channels
	uint8_t m_channelIRQFlags; // IRQ flags for individual channels
	bool m_forceIRQ; // When set the interrupt is active unconditionally ( even if 'masterIRQEnabled' is false )

	uint8_t m_IRQDummy;// Bits [0:5] of the interrupt registers are RW

	// The 7 channel instances
	// Channel 0: Media Decoder input
	// Channel 1: Media Decoder output
	// Channel 2: GPU
	// Channel 3: CDROM drive
	// Channel 4: SPU
	// Channel 5: extension port
	// Channel 6: RAM, used to clear an "ordering table"
	Channel m_channels[7];
};
