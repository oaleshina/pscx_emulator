#include "pscx_gamepad.h"
#include "pscx_common.h"

// ********************** GamePad implementation **********************
GamePad::GamePad(Type padType) :
	m_profile(nullptr),
	m_seq(0x0),
	m_active(true)
{
	switch (padType)
	{
	case Type::TYPE_DISCONNECTED:
		m_profile = new DisconnectedProfile;
		break;
	case Type::TYPE_DIGITAL:
		m_profile = new DigitalProfile;
	}
}

void GamePad::setSelect()
{
	// Prepare for incomming command
	m_active = true;
	m_seq = 0x0;
}

std::pair<uint8_t, bool> GamePad::sendCommand(uint8_t cmd)
{
	if (!m_active)
		return std::make_pair(0xff, false);

	std::pair<uint8_t, bool> command = m_profile->handleCommand(m_seq, cmd);
	
	// If we are not asserting DSR, it either means that we've encountered an error
	// or that we have nothing else to reply. In another case we won't be handling more command
	// bytes in this transaction.
	m_active = command.second;
	m_seq += 1;

	return command;
}

Profile* GamePad::getProfile() const
{
	return m_profile;
}

// ********************** DisconnectedProfile implementation **********************
std::pair<uint8_t, bool> DisconnectedProfile::handleCommand(uint8_t seq, uint8_t cmd)
{
	PCSX_UNUSED(seq, cmd);
	// The bus is open, no response
	return std::make_pair(0xff, false);
}

void DisconnectedProfile::setButtonState(Button button, ButtonState state)
{
	PCSX_UNUSED(button, state);
	// Dummy function
}

// ********************** DigitalProfile implementation **********************
std::pair<uint8_t, bool> DigitalProfile::handleCommand(uint8_t seq, uint8_t cmd)
{
	switch (seq)
	{
	// First byte should be 0x01 if the command targets the controller.
	case 0:
		return std::make_pair(0xff, cmd == 0x01);
	// Digital gamepad only supports command 0x42: read buttons.
	// Response 0x41: we're a digital PSX controller.
	case 1:
		return std::make_pair(0x41, cmd == 0x42);
	// The command byte is ignored.
	// Response 0x5a: 2nd controller ID byte.
	case 2:
		return std::make_pair(0x5a, true);
	// First button state byte: direction cross, start and select.
	case 3:
		return std::make_pair((uint8_t)m_digitalProfile, true);
	// 2nd button state byte: shoulder buttons and "shape" buttons.
	// We don't assert DSR for the last byte.
	case 4:
		return std::make_pair((uint8_t)(m_digitalProfile >> 8), false);
	}

	// Shouldn't be reached
	return std::make_pair(0xff, false);
}

void DigitalProfile::setButtonState(Button button, ButtonState state)
{
	size_t mask = 1ULL << button;
	if (state == ButtonState::BUTTON_STATE_PRESSED)
	{
		m_digitalProfile &= ~mask;
	}
	else if (state == ButtonState::BUTTON_STATE_RELEASED)
	{
		m_digitalProfile |= mask;
	}
}
