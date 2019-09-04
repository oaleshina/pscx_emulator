#include <iostream>
#include <cassert>

#include "pscx_interconnect.h"

Interconnect::Interconnect(Bios bios, HardwareType hardwareType, const Disc* disc) :
	m_irqState(new InterruptState),
	m_bios(new Bios(bios)),
	m_ram(new Ram),
	m_scratchPad(new ScratchPad),
	m_dma(new Dma),
	m_gpu(new Gpu(hardwareType)),
	m_spu(new Spu),
	m_timers(new Timers),
	m_cacheControl(new CacheControl(0x0)),
	m_cdRom(new CdRom(disc)),
	m_padMemCard(new PadMemCard),
	m_ramSize(0x0)
{
	memset(m_memControl, 0x0, sizeof(m_memControl));
}

template<typename T>
Instruction Interconnect::load(TimeKeeper& timeKeeper, uint32_t addr)
{
	// timeKeeper.tick(5);

	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (BIOS.contains(targetPeripheralAddress, offset))
		return Instruction(m_bios->load<T>(offset));

	if (RAM.contains(targetPeripheralAddress, offset))
		return Instruction(m_ram->load<T>(offset));

	if (SCRATCH_PAD.contains(targetPeripheralAddress, offset))
	{
		assert(addr <= 0xa0000000, "ScratchPad access through uncached memory");
		return Instruction(m_scratchPad->load<T>(offset));
	}

	if (MEM_CONTROL.contains(targetPeripheralAddress, offset))
	{
		assert((std::is_same<uint32_t, T>::value), "Unhandled MEM_CONTROL access");
		return Instruction(m_memControl[offset >> 2]);
	}

	if (SPU.contains(targetPeripheralAddress, offset))
		return Instruction(m_spu->load<T>(offset));

	if (RAM_SIZE.contains(targetPeripheralAddress, offset))
		return Instruction(m_ramSize);

	if (PAD_MEMCARD.contains(targetPeripheralAddress, offset))
		return Instruction(m_padMemCard->load<T>(timeKeeper, *m_irqState, offset));

	if (EXPANSION_1.contains(targetPeripheralAddress, offset))
		return Instruction(~0);

	if (EXPANSION_2.contains(targetPeripheralAddress, offset))
		return Instruction(~0, Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED);

	if (IRQ_CONTROL.contains(targetPeripheralAddress, offset))
	{
		uint32_t irqControlValue = 0x0;
		if (offset == 0x0)
			irqControlValue = m_irqState->getInterruptStatus();
		else if (offset = 0x4)
			irqControlValue = m_irqState->getInterruptMask();
		else
			assert(0, "Unhandled IRQ load");
		return Instruction(irqControlValue);
	}

	if (DMA.contains(targetPeripheralAddress, offset))
	{
		return Instruction(getDmaRegister<T>(offset));
	}

	if (GPU.contains(targetPeripheralAddress, offset))
	{
		return Instruction(m_gpu->load<T>(timeKeeper, *m_irqState, offset));
	}

	if (CDROM.contains(targetPeripheralAddress, offset))
	{
		return Instruction(m_cdRom->load<T>(timeKeeper, *m_irqState, offset));
	}

	if (TIMERS.contains(targetPeripheralAddress, offset))
	{
		return Instruction(m_timers->load<T>(timeKeeper, *m_irqState, offset));
	}

	if (MDEC.contains(targetPeripheralAddress, offset))
	{
		LOG("Unhandled load from MDEC register 0x" << std::hex << offset);
		return Instruction(0);
	}

	LOG("Unhandled fetch32 at address 0x" << std::hex << addr);
	return Instruction(~0, Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH);
}

template Instruction Interconnect::load<uint32_t>(TimeKeeper&, uint32_t);
template Instruction Interconnect::load<uint16_t>(TimeKeeper&, uint32_t);
template Instruction Interconnect::load<uint8_t> (TimeKeeper&, uint32_t);

template<typename T>
void Interconnect::store(TimeKeeper& timeKeeper, uint32_t addr, T value)
{
	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (MEM_CONTROL.contains(targetPeripheralAddress, offset))
	{
		assert((std::is_same<uint32_t, T>::value), "Unhandled MEM_CONTROL access");

		switch (offset)
		{
		case 0:
		{
			// Expansion 1 base address
			assert(value == 0x1f000000, "Bad expansion 1 base address");
			break;
		}
		case 4:
		{
			// Expansion 2 base address
			assert(value == 0x1f802000, "Bad expansion 2 base address");
			break;
		}
		default:
			LOG("Unhandled write to MEM_CONTROL register");
			return;
		}

		m_memControl[offset >> 2] = value;
		return;
	}

	if (RAM.contains(targetPeripheralAddress, offset))
	{
		m_ram->store<T>(offset, value);
		return;
	}

	if (SCRATCH_PAD.contains(targetPeripheralAddress, offset))
	{
		assert(addr <= 0xa0000000, "ScratchPad access through uncached memory");
		m_scratchPad->store<T>(offset, value);
		return;
	}

	if (RAM_SIZE.contains(targetPeripheralAddress, offset))
	{
		assert((std::is_same<uint32_t, T>::value), "Unhandled RAM_SIZE access");
		m_ramSize = value;
		return;
	}

	if (PAD_MEMCARD.contains(targetPeripheralAddress, offset))
	{
		m_padMemCard->store<T>(timeKeeper, *m_irqState, offset, value);
		return;
	}

	if (CACHE_CONTROL.contains(targetPeripheralAddress, offset))
	{
		assert((std::is_same<uint32_t, T>::value), "Unhandled cache control access");
		*m_cacheControl = CacheControl(value);
		return;
	}

	if (IRQ_CONTROL.contains(targetPeripheralAddress, offset))
	{
		if (offset == 0x0)
			m_irqState->acknowledgeInterrupts(value);
		else if (offset == 0x4)
			m_irqState->setInterruptMask(value);
		else
			assert(0, "Unhandled IRQ store");
		return;
	}

	if (DMA.contains(targetPeripheralAddress, offset))
	{
		setDmaRegister<T>(offset, value);
		return;
	}

	if (GPU.contains(targetPeripheralAddress, offset))
	{
		m_gpu->store<T>(timeKeeper, *m_timers, *m_irqState, offset, value);
		return;
	}

	if (CDROM.contains(targetPeripheralAddress, offset))
	{
		m_cdRom->store<T>(timeKeeper, *m_irqState, offset, value);
		return;
	}

	if (MDEC.contains(targetPeripheralAddress, offset))
	{
		LOG("Unhandled write to MDEC register 0x" << std::hex << offset);
		return;
	}

	if (TIMERS.contains(targetPeripheralAddress, offset))
	{
		m_timers->store<T>(timeKeeper, *m_irqState, *m_gpu, offset, value);
		return;
	}

	if (SPU.contains(targetPeripheralAddress, offset))
	{
		m_spu->store<T>(offset, value);
		return;
	}

	if (EXPANSION_2.contains(targetPeripheralAddress, offset))
	{
		LOG("Unaligned write to expansion 2 register 0x" << std::hex << offset);
		return;
	}

	LOG("Unhandled store32 into address 0x" << std::hex << targetPeripheralAddress);
}

template void Interconnect::store<uint32_t>(TimeKeeper&, uint32_t, uint32_t);
template void Interconnect::store<uint16_t>(TimeKeeper&, uint32_t, uint16_t);
template void Interconnect::store<uint8_t> (TimeKeeper&, uint32_t, uint8_t );

template<typename T>
T Interconnect::getDmaRegister(uint32_t offset) const
{
	assert((std::is_same<uint32_t, T>::value), "Unhandled DMA load");

	uint32_t major = (offset & 0x70) >> 4;
	uint32_t minor = offset & 0xf;

	// Per-channel registers
	if (major >= 0 && major <= 6)
	{
		const Channel& channel = m_dma->getDmaChannelRegister((Port)major);
		if (minor == 0x0)
			return channel.getBaseAddress();
		else if (minor == 0x4)
			return channel.getBlockControlRegister();
		else if (minor == 0x8)
			return channel.getDmaChannelControlRegister();
		else
			assert(0, "Unhandled DMA read");
	}
	// Common DMA registers
	else if (major == 0x7)
	{
		if (minor == 0x0)
			return m_dma->getDmaControlRegister();
		else if (minor == 0x4)
			return m_dma->getDmaInterruptRegister();
		else
			assert(0, "Unhandled DMA read");
	}

	/*switch(offset)
	{
	case 0x70:
		return m_dma.getDmaControlRegister();
	case 0x74:
		return m_dma.getDmaInterruptRegister();
	}*/

	LOG("Unhandled DMA read at 0x" << std::hex << offset);
	return ~0;
}

template<typename T>
void Interconnect::setDmaRegister(uint32_t offset, T value)
{
	assert((std::is_same<uint32_t, T>::value), "Unhandled DMA store");

	uint32_t major = (offset & 0x70) >> 4;
	uint32_t minor = offset & 0xf;

	Port activePort = (Port)-1;

	// Per-channel registers
	if (major >= 0 && major <= 6)
	{
		Channel& channel = m_dma->getDmaChannelRegisterMutable((Port)major);
		if (minor == 0x0)
			channel.setBaseAddress(value);
		else if (minor == 0x4)
			channel.setBlockControlRegister(value);
		else if (minor == 0x8)
			channel.setDmaChannelControlRegister(value);
		else
			assert(0, "Unhandled DMA write");
		
		if (channel.isActiveChannel())
			activePort = (Port)major;
	}
	// Common DMA registers
	else if (major == 0x7)
	{
		if (minor == 0x0)
			m_dma->setDmaControlRegister(value);
		else if (minor == 0x4)
			m_dma->setDmaInterruptRegister(value, *m_irqState);
		else
			assert(0, "Unhandled DMA write");
	}

	/*switch(offset)
	{
	case 0x70:
		m_dma.setDmaControlRegister(value);
		return;
	case 0x74:
		m_dma.setDmaInterruptRegister(value);
		return;
	}*/

	if (activePort != (Port)-1)
		doDma(activePort);

	LOG("Unhandled DMA write 0x" << std::hex << offset << " 0x" << value);
}

void Interconnect::doDma(Port port)
{
	// DMA transfer has been started, for now let's process everything in one pass
	// ( i.e. no chopping or priority handling )

	if (m_dma->getDmaChannelRegister(port).getSync() == Sync::SYNC_LINKED_LIST)
	{
		doDmaLinkedList(port);
	}
	else
	{
		doDmaBlock(port);
	}
	m_dma->done(port, *m_irqState);
}

void Interconnect::doDmaBlock(Port port)
{
	Channel& channel = m_dma->getDmaChannelRegisterMutable(port);

	uint32_t increment = 0;
	if (channel.getStep() == Step::STEP_INCREMENT) increment = 4;
	else if (channel.getStep() == Step::STEP_DECREMENT) increment = (uint32_t)-4;

	uint32_t addr = channel.getBaseAddress();

	// Transfer size in words.
	// Shouldn't happen since we shouldn't reach this code in linked list mode.
	assert(channel.getSync() != Sync::SYNC_LINKED_LIST, "Couldn't figure out DMA block transfer size");

	uint32_t transferSize = channel.getTransferSize();
	while (transferSize > 0)
	{
		// The two LSBs are ignored
		uint32_t currentAddr = addr & 0x1ffffc;

		switch (channel.getDirection())
		{
		case Direction::DIRECTION_FROM_RAM:
		{
			uint32_t srcWord = m_ram->load<uint32_t>(currentAddr);

			if (port == Port::PORT_GPU)
			{
				m_gpu->gp0(srcWord);
			}
			else if (port == Port::PORT_MDEC_IN)
			{
				// No implementation for now
				LOG("MDEC port");
			}
			else if (port == Port::PORT_SPU)
			{
				// Ignore transfers to the SPU for now
				LOG("SPU port");
			}
			else
			{
				//std::cout << port << std::endl;
				assert(0, "Unhandled DMA destination port");
			}
			break;
		}
		case Direction::DIRECTION_TO_RAM:
		{
			uint32_t srcWord = 0;
			// Clear ordering table
			if (port == Port::PORT_OTC)
			{
				if (transferSize == 1)
					srcWord = 0xffffff; // Last entry contains the end of the table marker
				else
					srcWord = (addr - 4) & 0x1fffff; // Pointer to the previous entry
			}
			else if (port == Port::PORT_GPU)
			{
				LOG("DMA GPU READ");
				srcWord = 0;
			}
			else if (port == Port::PORT_CD_ROM)
			{
				srcWord = m_cdRom->dmaReadWord();
			}
			else
			{
				assert(0, "Unhandled DMA source port");
			}
			m_ram->store<uint32_t>(currentAddr, srcWord);
		}
		}
		addr += increment;
		transferSize -= 1;
	}
}

void Interconnect::doDmaLinkedList(Port port)
{
	Channel& channel = m_dma->getDmaChannelRegisterMutable(port);

	uint32_t addr = channel.getBaseAddress() & 0x1ffffc;

	assert(channel.getDirection() != Direction::DIRECTION_TO_RAM, "Invalid DMA direction for linked list mode");
	assert(port == Port::PORT_GPU, "Attempted linked list DMA on port");

	while (true)
	{
		// In linked list mode, each entry starts with a "header" word. The high byte contains
		// the number of words in the "packet" ( not counting the header word )
		uint32_t header = m_ram->load<uint32_t>(addr);

		uint32_t remsz = header >> 24;
		while (remsz > 0)
		{
			addr = (addr + 4) & 0x1ffffc;

			uint32_t command = m_ram->load<uint32_t>(addr);
			
			// Send command to the GPU
			m_gpu->gp0(command);
			
			remsz -= 1;
		}

		// The end-of-table marker is usually 0xffffff but mednafen only checks
		// for MSB.
		if (header & 0x800000)
		{
			break;
		}
		addr = header & 0x1ffffc;
	}
}

void Interconnect::sync(TimeKeeper& timeKeeper)
{
	if (timeKeeper.needsSync(Peripheral::PERIPHERAL_GPU))
		m_gpu->sync(timeKeeper, *m_irqState);
	if (timeKeeper.needsSync(Peripheral::PERIPHERAL_PAD_MEMCARD))
		m_padMemCard->sync(timeKeeper, *m_irqState);
	m_timers->sync(timeKeeper, *m_irqState);
	if (timeKeeper.needsSync(Peripheral::PERIPHERAL_CDROM))
		m_cdRom->sync(timeKeeper, *m_irqState);
}

CacheControl Interconnect::getCacheControl() const
{
	return *m_cacheControl;
}

template<typename T>
Instruction Interconnect::loadInstruction(uint32_t pc)
{
	uint32_t targetPeripheralAddress = maskRegion(pc);

	uint32_t offset = 0;
	if (RAM.contains(targetPeripheralAddress, offset))
		return Instruction(m_ram->load<T>(offset));

	if (BIOS.contains(targetPeripheralAddress, offset))
		return Instruction(m_bios->load<T>(offset));

	LOG("Unhandled instruction load at address 0x" << std::hex << pc);
	return Instruction(~0, Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH);
}

template Instruction Interconnect::loadInstruction<uint32_t>(uint32_t);

InterruptState Interconnect::getIrqState() const
{
	return *m_irqState;
}

std::vector<Profile*> Interconnect::getPadProfiles()
{
	return m_padMemCard->getPadProfiles();
}
