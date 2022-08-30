#pragma once

#include <algorithm>

// GamePad types supported by the emulator
enum class Type
{
	// No gamepad connected
	TYPE_DISCONNECTED,
	// SCPH-1080: original gamepad without analog sticks
	TYPE_DIGITAL
};

enum Button
{
	BUTTON_SELECT = 0,
	BUTTON_START = 3,
	BUTTON_DUP = 4,
	BUTTON_DRIGHT = 5,
	BUTTON_DDOWN = 6,
	BUTTON_DLEFT = 7,
	BUTTON_L2 = 8,
	BUTTON_R2 = 9,
	BUTTON_L1 = 10,
	BUTTON_R1 = 11,
	BUTTON_TRIANGLE = 12,
	BUTTON_CIRCLE = 13,
	BUTTON_CROSS = 14,
	BUTTON_SQUARE = 15
};

enum class ButtonState
{
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_RELEASED
};

// Struct is used to abstract the various controller types.
struct Profile
{
	// Handle a command byte sent by the console.
	// Returns a pair <response, dsr>. If dsr is false, the subsequent command bytes will be
	// ignored for the current transaction.
	virtual std::pair<uint8_t, bool> handleCommand(uint8_t seq, uint8_t cmd) = 0;

	// Set a button's state. The function can be called several times
	// in a row with the same button and the same state, it should be idempotent.
	virtual void setButtonState(Button button, ButtonState state) = 0;
};

// Dummy profile emulating an empty pad slot
struct DisconnectedProfile : public Profile
{
	std::pair<uint8_t, bool> handleCommand(uint8_t seq, uint8_t cmd) override;
	void setButtonState(Button button, ButtonState state) override;
};

// SCPH-1080: Digital gamepad.
// Full state is only two bytes since we only need one bit per button.
struct DigitalProfile : public Profile
{
	DigitalProfile() : m_digitalProfile(0xffff) {};

	std::pair<uint8_t, bool> handleCommand(uint8_t seq, uint8_t cmd) override;
	void setButtonState(Button button, ButtonState state) override;
private:
	uint16_t m_digitalProfile;
};

struct GamePad
{
	GamePad(Type padType);

	// Called when the "select" line goes down.
	void setSelect();

	// The first return value is true if the gamepad issues a dsr
	// pulse after the byte is read to notify the controller that
	// more data can be read. The 2nd return value is the response byte.
	std::pair<uint8_t, bool> sendCommand(uint8_t cmd);

	// Return a mutable reference to the underlying gamepad Profile
	Profile* getProfile() const;

private:
	// Gamepad profile
	Profile* m_profile;
	// Counter that is keeping track of the current position in the reply sequence
	uint8_t m_seq;
	// False if the pad is done proessing the current command
	bool m_active;
};
