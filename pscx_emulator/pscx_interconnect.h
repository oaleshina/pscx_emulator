#pragma once

#include "pscx_common.h"
#include "pscx_bios.h"
#include "pscx_ram.h"
#include "pscx_dma.h"
#include "pscx_gpu.h"
#include "pscx_memory.h"
#include "pscx_instruction.h"
#include "pscx_timekeeper.h"
#include "pscx_interrupts.h"
#include "pscx_timers.h"
#include "pscx_cdrom.h"

using namespace pscx_memory;

// Global interconnect
struct Interconnect
{
	Interconnect(Bios bios);

	template<typename T>
	Instruction load(TimeKeeper& timeKeeper, uint32_t addr);

	template<typename T>
	void store(TimeKeeper& timeKeeper, uint32_t addr, T value);

	template<typename T>
	T getDmaRegister(uint32_t offset) const; // DMA register read

	template<typename T>
	void setDmaRegister(uint32_t offset, T value); // DMA register write

	void doDma(Port port); // Execute DMA transfer for a port
	void doDmaBlock(Port port);
	void doDmaLinkedList(Port port); // Emulate DMA transfer for linked list synchronization mode

	void sync(TimeKeeper& timeKeeper);
	CacheControl getCacheControl() const;

	// Load instruction at 'PC'. Only RAM and BIOS are supported.
	template<typename T>
	Instruction loadInstruction(uint32_t pc);

	InterruptState getIrqState() const;

private:
	
	InterruptState m_irqState;
	Bios m_bios; // Basic Input/Output memory
	Ram m_ram; // Main RAM
	Dma m_dma; // DMA registers
	Gpu m_gpu; // Graphics Processir Unit
	Timers m_timers; // System timers
	CacheControl m_cacheControl; // Cache Control register
	CdRom m_cdRom; // CDROM controller
};