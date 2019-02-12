#pragma once

#include <cstdint>
#include <string.h>

#include "pscx_timekeeper.h"
#include "pscx_interrupts.h"

// Various IRQ codes used by the CDROM controller and their
// signification
enum IrqCode
{
	IRQ_CODE_OK = 3
};

// 16 byte FIFO used to store command arguments and results
struct Fifo
{
	Fifo() :
		m_writeIdx(0x0),
		m_readIdx(0x0)
	{
		memset(m_buffer, 0x0, sizeof(m_buffer));
	}

	bool isEmpty() const;
	bool isFull() const;
	void clear();

	// Retrieve the number of elements in the FIFO. This number is in
	// the range [0; 31].
	uint8_t len() const;

	void push(uint8_t value);
	uint8_t pop();

private:
	// Data buffer
	uint8_t m_buffer[16];

	// Write pointer (4 bits + carry)
	uint8_t m_writeIdx;

	// Read pointer (4 bits + carry)
	uint8_t m_readIdx;
};

// CDROM Controller
struct CdRom
{
	CdRom() :
		m_index(0x0),
		m_irqMask(0x0),
		m_irqFlags(0x0)
	{}

	template<typename T>
	T load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset);

	template<typename T>
	void store(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset, T value);

	uint8_t getStatus();

	bool irq() const;
	void triggerIrq(IrqCode irq);

	void setIndex(uint8_t index);
	void irqAck(uint8_t value);
	void irqMask(uint8_t value);

	void command(uint8_t cmd);
	void cmdGetStat();
	void cmdTest();
	void testVersion();

	void pushParam(uint8_t param);

private:
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
};
