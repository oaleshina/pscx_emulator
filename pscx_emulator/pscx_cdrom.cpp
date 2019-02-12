#include <cassert>

#include "pscx_cdrom.h"
#include "pscx_common.h"

// ***************** Fifo implementation *********************
bool Fifo::isEmpty() const
{
	// If both pointers point at the same cell and have the same carry the
	// FIFO is empty.
	return ((m_writeIdx ^ m_readIdx) & 0x1f) == 0;
}

bool Fifo::isFull() const
{
	// The FIFO is full if both indexes point to the same cell
	// while having a different carry.
	return ((m_readIdx ^ m_writeIdx ^ 0x10) & 0x1f) == 0;
}

void Fifo::clear()
{
	m_readIdx = m_writeIdx;
	memset(m_buffer, 0x0, sizeof(m_buffer));
}

uint8_t Fifo::len() const
{
	return (m_writeIdx - m_readIdx) & 0x1f;
}

void Fifo::push(uint8_t value)
{
	m_buffer[m_writeIdx & 0xf] = value;
	m_writeIdx += 1;
}

uint8_t Fifo::pop()
{
	uint8_t idx = m_readIdx & 0xf;
	m_readIdx += 1;
	return m_buffer[idx];
}

// ***************** CdRom implementation *********************
template<typename T>
T CdRom::load(TimeKeeper& timeKeeper, InterruptState& irqState, uint32_t offset)
{
	assert((std::is_same<uint8_t, T>::value), "Unhandled CDROM load");

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
		if (m_index == 0x1)
			value = m_irqFlags;
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

	// All writeable registers are 8 bit wide
	bool prevIrq = irq();

	switch (offset)
	{
	case 0x0:
		setIndex((uint8_t)value);
		break;
	case 0x1:
		if (m_index == 0x0)
			command((uint8_t)value);
		break;
	case 0x2:
		if (m_index == 0x0)
			pushParam((uint8_t)value);
		else if (m_index == 0x1)
			irqMask((uint8_t)value);
		break;
	case 0x3:
		if (m_index == 0x1)
		{
			irqAck((uint8_t)value & 0x1f);
			if (value & 0x40)
				m_params.clear();

			assert((value & 0xa0) == 0x0, "Unhandled CDROM 3.1");
			break;
		}
	}

	if (!prevIrq && irq())
	{
		// Interrupt rising edge
		irqState.raiseAssert(Interrupt::INTERRUPT_CDROM);
	}
}

template void CdRom::store<uint32_t>(TimeKeeper&, InterruptState&, uint32_t, uint32_t);
template void CdRom::store<uint16_t>(TimeKeeper&, InterruptState&, uint32_t, uint16_t);
template void CdRom::store<uint8_t >(TimeKeeper&, InterruptState&, uint32_t, uint8_t );

uint8_t CdRom::getStatus()
{
	uint8_t status = m_index;

	status |= 0 << 2;
	status |= ((uint8_t)m_params.isEmpty()) << 3;
	status |= ((uint8_t)(!m_params.isFull())) << 4;
	status |= ((uint8_t)(!m_params.isEmpty())) << 5;
	status |= 0 << 6;
	status |= 0 << 7;

	return status;
}

bool CdRom::irq() const
{
	return m_irqFlags & m_irqMask;
}

void CdRom::triggerIrq(IrqCode irq)
{
	assert(m_irqFlags == 0x0, "Unsupported nested CDROM interrupt");
	m_irqFlags = (uint8_t)irq;
}

void CdRom::setIndex(uint8_t index)
{
	m_index = index & 3;
}

void CdRom::irqAck(uint8_t value)
{
	m_irqFlags &= (~value);
}

void CdRom::irqMask(uint8_t value)
{
	m_irqMask = value & 0x1f;
}

void CdRom::command(uint8_t cmd)
{
	m_response.clear();

	switch (cmd)
	{
	case 0x01:
		cmdGetStat();
		break;
	case 0x19:
		cmdTest();
		break;
	default:
		assert(0, "Unhandled CDROM command");
	}

	m_params.clear();
}

void CdRom::cmdGetStat()
{
	// If this occurs on the real hardware it should set bit 1 of the result byte
	// and then put a 2nd byte 0x20 to signal the wrong number of params.
	assert(m_params.isEmpty(), "Unexpected parmeters for CDROM GetStat command");

	m_response.push(0x10);
	triggerIrq(IrqCode::IRQ_CODE_OK);
}

void CdRom::cmdTest()
{
	assert(m_params.len() == 0x1, "Unexpected number of parameters for CDROM test command");

	switch (m_params.pop())
	{
	case 0x20:
		testVersion();
	default:
		// Fails on assert. Used for debugging.
		// assert(0, "Unhandled CDROM test subcommand");
		break;
	}
}

void CdRom::testVersion()
{
	// Values returned by PAL SCPH-7502:
	// Year
	m_response.push(0x98);
	// Month
	m_response.push(0x06);
	// Day
	m_response.push(0x10);
	// Version
	m_response.push(0xc3);

	triggerIrq(IrqCode::IRQ_CODE_OK);
}

void CdRom::pushParam(uint8_t param)
{
	if (m_params.isFull())
		LOG("CDROM parameter FIFO overflow");
	m_params.push(param);
}
