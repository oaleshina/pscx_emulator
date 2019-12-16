#pragma once

#include <cstdint>
#include <string.h>
#include <vector>

#include "pscx_timekeeper.h"
#include "pscx_interrupts.h"
#include "pscx_disc.h"
#include "pscx_minutesecondframe.h"

// Various IRQ codes used by the CDROM controller and their
// signification.
enum IrqCode
{
	// A CD sector has been read and is ready to be processed.
	IRQ_CODE_SECTOR_READY = 1,
	// Command succesful, used for the 2nd response.
	IRQ_CODE_DONE = 2,
	// Command succesful, used for the 1st response.
	IRQ_CODE_OK = 3,
	// Error: invalid command, disc command while do disc is present.
	IRQ_CODE_ERROR = 5
};

// CDROM controller state machine.
enum CommandState
{
	// Controller is idle.
	COMMAND_STATE_IDLE,

	// Controller is issuing a command or waiting for the return value.
	// We store the number of cycles until the response is received ( RX delay )
	// and the number of cycles until the IRQ is triggered ( IRQ delay ) as well as the IRQ
	// code and response bytes in a FIFO. RX delay must always be less than or equal to IRQ delay.
	COMMAND_STATE_RX_PENDING,

	// Transaction is done but we're still waiting for the interrupt ( IRQ delay ).
	// For some reason it seems to occur a long time after the RX FIFO is filled
	// ( thousands of CPU cycles, at least with some commands ).
	COMMAND_STATE_IRQ_PENDING,

	// Invalid command state.
	COMMAND_STATE_INVALID
};

// CDROM data read state machine.
enum ReadState
{
	READ_STATE_IDLE,
	// We're expection a sector
	READ_STATE_READING
};

// 16 byte FIFO used to store command arguments and results.
struct Fifo
{
	Fifo() :
		m_writeIdx(0x0),
		m_readIdx(0x0)
	{
		memset(m_buffer, 0x0, sizeof(m_buffer));
	}

	static Fifo fromBytes(const std::vector<uint8_t>& bytes);

	bool isEmpty() const;
	bool isFull() const;
	void clear();

	// Retrieve the number of elements in the FIFO. This number is in
	// the range [0; 31].
	uint8_t len() const;

	void push(uint8_t value);
	uint8_t pop();

private:
	// Data buffer.
	uint8_t m_buffer[16];

	// Write pointer (4 bits + carry).
	uint8_t m_writeIdx;

	// Read pointer (4 bits + carry).
	uint8_t m_readIdx;
};

// CD-DA Audio playback mixer. The CDROM's audio stereo output can be
// mixed arbitrarily, before reaching the SPU stereo input.
struct Mixer
{
	Mixer() :
		m_cdLeftToSpuLeft(0x0),
		m_cdLeftToSpuRight(0x0),
		m_cdRightToSpuLeft(0x0),
		m_cdRightToSpuRight(0x0)
	{}
//private:
	uint8_t m_cdLeftToSpuLeft;
	uint8_t m_cdLeftToSpuRight;
	uint8_t m_cdRightToSpuLeft;
	uint8_t m_cdRightToSpuRight;
};

// CDROM Controller.
struct CdRom
{
	CdRom(const Disc* disc) :
		m_commandState(CommandState::COMMAND_STATE_IDLE),
		m_readState(ReadState::READ_STATE_IDLE),
		m_index(0x0),
		m_irqMask(0x0),
		m_irqFlags(0x0),
		m_onAcknowledge(&CdRom::ackIdle),
		m_disc(disc),
		m_seekTarget(MinuteSecondFrame::createZeroTimestamp()),
		m_seekTargetPending(false),
		m_readPosition(MinuteSecondFrame::createZeroTimestamp()),
		m_doubleSpeed(false),
		m_rxActive(false),
		m_rxIndex(0x0),
		m_rxOffset(0x0),
		m_rxLen(0x0),
		m_readWholeSector(true)
	{}

	template<typename T>
	T load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset);

	template<typename T>
	void store(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset, T value);

	void sync(TimeKeeper& timeKeeper, InterruptState& irqState);

	// Retrieve a single byte from the RX buffer.
	uint8_t readByte();

	// The DMA can read the RX buffer one word at a time.
	uint32_t dmaReadWord();

	void doSeek();

	// Retrieve the current disc or panic if there's none. Used in
	// functions that should not be reached if a disc is not present.
	// TODO: currently implemented without check if disc is successfully loaded.
	const Disc* getDiscOrDie();

	// The function is called when a new sector has been read.
	void sectorRead(InterruptState& irqState);

	uint8_t getStatus();

	bool irq() const;
	void triggerIrq(InterruptState& irqState, IrqCode irq);

	void setIndex(uint8_t index);
	void irqAck(uint8_t value);
	void setConfig(uint8_t config);
	void irqMask(uint8_t value);

	// Return the number of CPU cycles needed to read a single sector
	// depending on the current drive speed. The PSC drive can read 75 sectors
	// per second at 1x or 150 sectors per second at 2x.
	uint32_t getCyclesPerSector();

	void command(TimeKeeper& timeKeeper, uint8_t cmd);
	// Return the first status byte returned by many commands.
	uint8_t getDriveStatus() const;
	CommandState cmdGetStat();
	// Tell the CDROM controller where the next seek should take us
	// ( but don't physically perform the seek yet ).
	CommandState cmdSetLoc();
	// Start data read sequence, the controller will return sectors.
	CommandState cmdReadN();
	// Stop reading sectors, but remain at the same position on the disc.
	CommandState cmdPause();
	// Reinitialize the CD ROM controller.
	CommandState cmdInit();
	// Demute CDROM audio playback.
	CommandState cmdDemute();
	// Configure the behaviour of the CDROM drive
	CommandState cmdSetMode();
	// Execute seek. Target is given by previous SetLoc command.
	CommandState cmdSeekl();
	// Read the CD-ROM's identification string. This is how the BIOS checks
	// that the disc is an official PlayStation disc ( and not a copy ) and handles region
	// locking.
	CommandState cmdGetId();
	// Instruct the CD drive to read the table of contents.
	CommandState cmdReadToc();
	CommandState cmdTest();
	CommandState testVersion();

	// Placeholder function called when an interrupt is acknowledged
	// and the command is completed.
	CommandState ackIdle();
	CommandState ackSeekl();
	// Prepare the 2nd response of the GetId command.
	CommandState ackGetId();
	CommandState ackReadToc();
	CommandState ackPause();
	CommandState ackInit();

	void pushParam(uint8_t param);

private:
	// Command state machine.
	CommandState m_commandState;

	struct RxPending
	{
		RxPending () :
			m_rxDelay(0x0),
			m_irqDelay(0x0),
			m_irqCode(IrqCode::IRQ_CODE_OK)
		{}
		uint32_t m_rxDelay;
		uint32_t m_irqDelay;
		IrqCode m_irqCode;
		Fifo m_response;
	} m_helperRxPending;

	struct IrqPending
	{
		IrqPending() :
			m_irqDelay(0x0),
			m_irqCode(IrqCode::IRQ_CODE_OK)
		{}
		uint32_t m_irqDelay;
		IrqCode m_irqCode;
	} m_helperIrqPending;

	// Data read state machine.
	ReadState m_readState;

	struct Reading
	{
		Reading() :
			m_delay(0x0)
		{}
		uint32_t m_delay;
	} m_helperReading;

	// Some of the memory mapped registers change meaning depending
	// on the value of the index.
	uint8_t m_index;

	// Command arguments FIFO
	Fifo m_params;

	// Command response FIFO
	Fifo m_response;

	// Interrupt mask (5 bits)
	uint8_t m_irqMask;

	// Interrupt flag (5 bits)
	uint8_t m_irqFlags;

	// Commands/response are generally stalled as long as the interrupt is active.
	CommandState(CdRom::*m_onAcknowledge)(void);

	// Currently loaded disc or None if no disc is present.
	const Disc* m_disc;

	// Target of the next seek command.
	MinuteSecondFrame m_seekTarget;

	// True if m_seekTarget has been set but no seek took place.
	bool m_seekTargetPending;

	// Current read position.
	MinuteSecondFrame m_readPosition;

	// True, if the drive is in double speed mode ( 2x, 150 sectors per
	// second ), otherwise we're in the default 1x ( 75 sectors per second ).
	bool m_doubleSpeed;

	// Sector in the RX buffer.
	const XaSector* m_rxSector;

	// When this bit is set, the data RX buffer is active, otherwise it's reset.
	// The software is supposed to reset it between sectors.
	bool m_rxActive;

	// Index of the next byte to be read in the RX sector.
	uint16_t m_rxIndex;

	// Offset of m_rxIndex in the sector buffer.
	uint16_t m_rxOffset;
	// Index of the last byte to be read in the RX sector.
	uint16_t m_rxLen;

	// If true we read the whole sector except for the sync bytes
	// (0x924 bytes), otherwise it only reads 0x800 bytes.
	bool m_readWholeSector;

	// CDROM audio mixer connected to the SPU.
	Mixer m_mixer;
};
