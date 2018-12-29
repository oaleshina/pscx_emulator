#include "pscx_interconnect.h"

#include <iostream>

Interconnect::Interconnect(Bios bios) :
	m_bios(bios)
{
}

Instruction Interconnect::load32(uint32_t addr) const
{
	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (addr & 0b11)
	{
		LOG("Unaligned load32 address 0x" << std::hex << addr);
		return Instruction(~0, Instruction::INSTRUCTION_STATUS_UNALIGNED_ACCESS);
	}

	if (BIOS.contains(targetPeripheralAddress, offset))
		return Instruction(m_bios.load32(offset));

	if (RAM.contains(targetPeripheralAddress, offset))
		return Instruction(m_ram.load32(offset));

	if (MEM_CONTROL.contains(targetPeripheralAddress, offset))
		return Instruction(~0, Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED);

	if (SPU.contains(targetPeripheralAddress, offset))
		return Instruction(~0, Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED);

	if (RAM_SIZE.contains(targetPeripheralAddress, offset))
		return Instruction(~0, Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED);

	if (EXPANSION_1.contains(targetPeripheralAddress, offset))
		return Instruction(~0, Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED);

	if (EXPANSION_2.contains(targetPeripheralAddress, offset))
		return Instruction(~0, Instruction::INSTRUCTION_STATUS_NOT_IMPLEMENTED);

	if (IRQ_CONTROL.contains(targetPeripheralAddress, offset))
	{
		LOG("IRQ control read 0x" << std::hex << offset);
		return Instruction(0);
	}

	if (DMA.contains(targetPeripheralAddress, offset))
	{
		return Instruction(getDmaRegister(offset));
	}

	if (GPU.contains(targetPeripheralAddress, offset))
	{
		LOG("GPU read 0x" << std::hex << offset);

		// GPUSTAT: set bit 26, 27, 28 to signal that the GPU is ready for DMA and CPU access.
		// This way the BIOS won't dead lock waiting for an event that will never come
		if (offset == 0x0)
		{
			return Instruction(m_gpu.getReadRegister());
		}
		if (offset == 0x4)
		{
			return Instruction(m_gpu.getStatusRegister());
		}

		return Instruction(0);
	}

	if (TIMERS.contains(targetPeripheralAddress, offset))
	{
		LOG("Unhandled read from the timer register 0x" << std::hex << offset);
		return Instruction(0);
	}
	
	LOG("Unhandled fetch32 at address 0x" << std::hex << addr);
	return Instruction(~0, Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH);
}

Instruction Interconnect::load16(uint32_t addr) const
{
	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (SPU.contains(targetPeripheralAddress, offset))
	{
		LOG("Unhandled read from SPU register 0x" << std::hex << targetPeripheralAddress);
		return Instruction(0);
	}

	if (RAM.contains(targetPeripheralAddress, offset))
	{
		return Instruction(m_ram.load16(offset));
	}

	if (IRQ_CONTROL.contains(targetPeripheralAddress, offset))
	{
		LOG("IRQ control read 0x" << std::hex << offset);
		return Instruction(0);
	}

	LOG("Unhandled fetch16 at address 0x" << std::hex << addr);
	return Instruction(~0, Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH);
}

Instruction Interconnect::load8(uint32_t addr) const
{
	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (RAM.contains(targetPeripheralAddress, offset))
		return Instruction(m_ram.load8(offset));

	if (BIOS.contains(targetPeripheralAddress, offset))
		return Instruction(m_bios.load8(offset));

	if (EXPANSION_1.contains(targetPeripheralAddress, offset))
	{
		// No expansion implemented
		return Instruction(0xff);
	}

	LOG("Unhandled fetch8 at address 0x" << std::hex << addr);
	return Instruction(~0, Instruction::INSTRUCTION_STATUS_UNHANDLED_FETCH);
}

void Interconnect::store32(uint32_t addr, uint32_t value)
{
	if (addr & 0b11)
	{
		LOG("Unaligned store32 address 0x" << std::hex << addr);
		return;
	}

	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (MEM_CONTROL.contains(targetPeripheralAddress, offset))
	{
		switch (offset)
		{
		case 0:
		{
			// Expansion 1 base address
			if (value != 0x1f000000)
			{
				LOG("Bad expansion 1 base address 0x" << std::hex << value);
				return;
			}
			break;
		}
		case 4:
		{
			// Expansion 2 base address
			if (value != 0x1f802000)
			{
				LOG("Bad expansion 2 base address 0x" << std::hex << value);
				return;
			}
			break;
		}
		default:
			LOG("Unhandled write to MEM_CONTROL register");
			return;
		}
		return;
	}

	if (RAM.contains(targetPeripheralAddress, offset))
	{
		m_ram.store32(offset, value);
		return;
	}

	if (RAM_SIZE.contains(targetPeripheralAddress, offset))
	{
		LOG("Ignore store to RAM_SIZE register for now");
		return;
	}

	if (CACHE_CONTROL.contains(targetPeripheralAddress, offset))
	{
		LOG("Ignore store to CACHE_CONTROL register for now");
		return;
	}

	if (IRQ_CONTROL.contains(targetPeripheralAddress, offset))
	{
		LOG("IRQ control 0x" << std::hex << offset << " 0x" << value);
		return;
	}

	if (DMA.contains(targetPeripheralAddress, offset))
	{
		setDmaRegister(offset, value);
		return;
	}

	if (GPU.contains(targetPeripheralAddress, offset))
	{
		if (offset == 0x0)
		{
			m_gpu.gp0(value);
		}
		else if (offset == 0x4)
		{
			m_gpu.gp1(value);
		}
		else
		{
			LOG("GPU write 0x" << std::hex << offset << " 0x" << value);
		}
		return;
	}

	if (TIMERS.contains(targetPeripheralAddress, offset))
	{
		LOG("Unhandled write to timer register 0x" << std::hex << offset << " 0x" << value);
		int a = 1;
		return;
	}

	LOG("Unhandled store32 into address 0x" << std::hex << targetPeripheralAddress);
}

void Interconnect::store16(uint32_t addr, uint16_t value)
{
	if (addr & 1)
	{
		LOG("Unaligned store16 address 0x" << std::hex << addr);
		return;
	}

	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (SPU.contains(targetPeripheralAddress, offset))
	{
		LOG("Unaligned write to SPU register 0x" << std::hex << offset);
		return;
	}
	
	if (TIMERS.contains(targetPeripheralAddress, offset))
	{
		LOG("Unhandled write to timer register 0x" << std::hex << offset);
		return;
	}

	if (RAM.contains(targetPeripheralAddress, offset))
	{
		m_ram.store16(offset, value);
		return;
	}

	if (IRQ_CONTROL.contains(targetPeripheralAddress, offset))
	{
		LOG("IRQ control write 0x" << std::hex << offset << " 0x" << value);
		return;
	}

	LOG("Unhandled store16 into address 0x" << std::hex << targetPeripheralAddress);
}

void Interconnect::store8(uint32_t addr, uint8_t value)
{
	uint32_t targetPeripheralAddress = maskRegion(addr);

	uint32_t offset = 0;
	if (RAM.contains(targetPeripheralAddress, offset))
	{
		m_ram.store8(offset, value);
		return;
	}

	if (EXPANSION_2.contains(targetPeripheralAddress, offset))
	{
		LOG("Unaligned write to expansion 2 register 0x" << std::hex << offset);
		return;
	}

	if (CDROM.contains(targetPeripheralAddress, offset))
	{
		LOG("Unaligned write to CDROM register 0x" << std::hex << offset);
		return;
	}

	LOG("Unhandled store8 into address 0x" << std::hex << targetPeripheralAddress);
}

uint32_t Interconnect::getDmaRegister(uint32_t offset) const
{
	uint32_t major = (offset & 0x70) >> 4;
	uint32_t minor = offset & 0xf;

	// Per-channel registers
	if (major >= 0 && major <= 6)
	{
		const Channel& channel = m_dma.getDmaChannelRegister((Port)major);
		if (minor == 0x0)
			return channel.getBaseAddress();
		else if (minor == 0x4)
			return channel.getBlockControlRegister();
		else if (minor == 0x8)
			return channel.getDmaChannelControlRegister();
		else
		{
			LOG("Unhandled DMA read at 0x" << std::hex << offset);
			return ~0;
		}
	}
	// Common DMA registers
	else if (major == 0x7)
	{
		if (minor == 0x0)
			return m_dma.getDmaControlRegister();
		else if (minor == 0x4)
			return m_dma.getDmaInterruptRegister();
		else
		{
			LOG("Unhandled DMA read at 0x" << std::hex << offset);
			return ~0;
		}
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

void Interconnect::setDmaRegister(uint32_t offset, uint32_t value)
{
	uint32_t major = (offset & 0x70) >> 4;
	uint32_t minor = offset & 0xf;

	Port activePort = (Port)-1;

	// Per-channel registers
	if (major >= 0 && major <= 6)
	{
		Channel& channel = m_dma.getDmaChannelRegisterMutable((Port)major);
		if (minor == 0x0)
			channel.setBaseAddress(value);
		else if (minor == 0x4)
			channel.setBlockControlRegister(value);
		else if (minor == 0x8)
			channel.setDmaChannelControlRegister(value);
		else
		{
			LOG("Unhandled DMA write 0x" << std::hex << offset << " 0x" << value);
			return;
		}
		
		if (channel.isActiveChannel())
			activePort = (Port)major;
	}
	// Common DMA registers
	else if (major == 0x7)
	{
		if (minor == 0x0)
			m_dma.setDmaControlRegister(value);
		else if (minor == 0x4)
			m_dma.setDmaInterruptRegister(value);
		else
		{
			LOG("Unhandled DMA write 0x" << std::hex << offset << " 0x" << value);
			return;
		}
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

	if (m_dma.getDmaChannelRegister(port).getSync() == Sync::SYNC_LINKED_LIST)
	{
		doDmaLinkedList(port);
	}
	else
	{
		doDmaBlock(port);
	}
}

void Interconnect::doDmaBlock(Port port)
{
	Channel& channel = m_dma.getDmaChannelRegisterMutable(port);

	int32_t increment = 0;
	if (channel.getStep() == Step::STEP_INCREMENT) increment = 4;
	else if (channel.getStep() == Step::STEP_DECREMENT) increment = -4;

	uint32_t addr = channel.getBaseAddress();

	// Transfer size in words
	if (channel.getSync() == Sync::SYNC_LINKED_LIST)
	{
		// Shouldn't happen since we shouldn't reach this code in linked list mode
		LOG("Couldn't figure out DMA block transfer size");
		return;
	}

	uint32_t transferSize = channel.getTransferSize();
	while (transferSize > 0)
	{
		// The two LSBs are ignored
		uint32_t currentAddr = addr & 0x1ffffc;

		switch (channel.getDirection())
		{
		case Direction::DIRECTION_FROM_RAM:
		{
			uint32_t srcWord = m_ram.load32(currentAddr);

			if (port == Port::PORT_GPU)
			{
				m_gpu.gp0(srcWord);
			}
			else
			{
				LOG("Unhandled DMA destination port 0x" << std::hex << port);
				return;
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
			else
			{
				LOG("Unhandled DMA source port 0x" << std::hex << port);
				return;
			}
			m_ram.store32(currentAddr, srcWord);
		}
		}
		addr += increment;
		transferSize -= 1;
	}
	channel.done();
}

void Interconnect::doDmaLinkedList(Port port)
{
	Channel& channel = m_dma.getDmaChannelRegisterMutable(port);

	uint32_t addr = channel.getBaseAddress() & 0x1ffffc;

	if (channel.getDirection() == Direction::DIRECTION_TO_RAM)
	{
		LOG("Invalid DMA direction for linked list mode");
		return;
	}

	if (port != Port::PORT_GPU)
	{
		LOG("Attempted linked list DMA on port 0x" << std::hex << port);
		return;
	}

	while (true)
	{
		// In linked list mode, each entry starts with a "header" word. The high byte contains
		// the number of words in the "packet" ( not counting the header word )
		uint32_t header = m_ram.load32(addr);

		uint32_t remsz = header >> 24;
		while (remsz > 0)
		{
			addr = (addr + 4) & 0x1ffffc;

			uint32_t command = m_ram.load32(addr);
			
			// Send command to the GPU
			m_gpu.gp0(command);
			
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
	channel.done();
}
