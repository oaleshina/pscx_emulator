#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "SDL.h"
#include "SDL_events.h"
#include "pscx_bios.h"
#include "pscx_cpu.h"
#include "pscx_interconnect.h"

struct ArgSetParser
{
	ArgSetParser(int argc, char** argv)
	{
		m_args.resize(argc);
		for (size_t i = 0; i < argc; ++i)
			m_args[i] = argv[i];
	}

	const std::vector<std::string>& args() { return m_args; }

private:
	std::vector<std::string> m_args;
};

static void printUsageAndExit(const char* argv0)
{
	std::cerr
	<< "Usage  : " << argv0 << " <Path to BIOS BIN> [CDROM-bin-file] [options]\n"
	<< "App options:\n"
	<< "  -h    | --help                        Print this usage message\n"
	<< "  -disc | --disc-bin-path               Path to disc location\n"
	<< "  -dump | --dump-instructions-registers Dump instructions and registers to the file\n"
	<< "  -rt   | --run-testing                 Compare output results with the golden file\n"
	<< std::endl;

	exit(1);
}

static void generateDumpOutputFn(const Cpu& cpu)
{
	std::ofstream dumpLog("dump_output.txt");

	const std::vector<uint32_t>& instructionsDump = cpu.getInstructionsDump();

	// Dump instructions
	dumpLog << instructionsDump.size() << " ";
	for (size_t i = 0; i < instructionsDump.size(); ++i)
		dumpLog << instructionsDump[i] << " ";

	const uint32_t* regs = cpu.getRegistersPtr();

	// Dump registers
	for (size_t i = 0; i < 32; ++i)
		dumpLog << regs[i] << " ";

	dumpLog.close();
}

static bool compareGoldenWithDump(const Cpu& cpu)
{
	std::ifstream goldenInput("golden/golden_result.txt");
	if (!goldenInput.good())
	{
		LOG("No golden file found");
		return false;
	}

	uint32_t instructionsCount = 0;
	goldenInput >> instructionsCount;

	const std::vector<uint32_t>& instructionsDump = cpu.getInstructionsDump();
	if (instructionsCount != instructionsDump.size())
		return false;

	// Compare opcodes
	uint32_t opcode = 0;
	for (size_t i = 0; i < instructionsCount; ++i)
	{
		goldenInput >> opcode;
		if (opcode != instructionsDump[i])
			return false;
	}

	const uint32_t* regs = cpu.getRegistersPtr();

	// Compare registers
	uint32_t registerValue = 0;
	for (size_t i = 0; i < 32; ++i)
	{
		goldenInput >> registerValue;
		if (registerValue != regs[i])
			return false;
	}

	goldenInput.close();
	return true;
}

// ***************** Controller settings *****************
enum Action
{
	ACTION_NONE,
	ACTION_QUIT,
	ACTION_DEBUG
};

static SDL_GameController* initializeSDL2Controllers()
{
	int numJoysticks = SDL_NumJoysticks();
	if (numJoysticks < 0)
	{
		// Error occured
		LOG("Can't enumerate joysticks, error code 0x" << std::hex << SDL_GetError());
		return nullptr;
	}

	SDL_GameController* initializedController = nullptr;
	for (int joystickId = 0; joystickId < numJoysticks; ++joystickId)
	{
		if (SDL_IsGameController(joystickId))
		{
			LOG("Attempt to open controller 0x" << std::hex << joystickId);

			SDL_GameController* controller = SDL_GameControllerOpen(joystickId);
			if (controller)
			{
				LOG("Successfully opened " << SDL_GameControllerName(joystick));
				initializedController = controller;
				break;
			}
			LOG("FAILED: " << SDL_GetError());
		}
	}

	if (initializedController)
		LOG("Controller suppport enabled");
	else
		LOG("No controller found");

	return initializedController;
}

static void handleKeyboard(Profile* pad, SDL_Keycode keyCode, ButtonState state)
{
	Button button;
	switch (keyCode)
	{
	case SDLK_RETURN:
		button = Button::BUTTON_START;
		break;
	case SDLK_RSHIFT:
		button = Button::BUTTON_SELECT;
		break;
	case SDLK_UP:
		button = Button::BUTTON_DUP;
		break;
	case SDLK_DOWN:
		button = Button::BUTTON_DDOWN;
		break;
	case SDLK_KP_2:
		button = Button::BUTTON_CROSS;
		break;
	case SDLK_KP_4:
		button = Button::BUTTON_SQUARE;
		break;
	case SDLK_KP_6:
		button = Button::BUTTON_CIRCLE;
		break;
	case SDLK_KP_7:
		button = Button::BUTTON_L1;
		break;
	case SDLK_KP_8:
		button = Button::BUTTON_TRIANGLE;
		break;
	case SDLK_NUMLOCKCLEAR:
		button = Button::BUTTON_L2;
		break;
	case SDLK_KP_9:
		button = Button::BUTTON_R1;
		break;
	case SDLK_KP_MULTIPLY:
		button = Button::BUTTON_R2;
		break;
	default:
		// Unhandled key
		return;
	}
	
	pad->setButtonState(button, state);
}

static void handleController(Profile* pad, SDL_ControllerButtonEvent& buttonEvent, ButtonState state)
{
	// Map the playstation controller on the xbox360 one
	Button button;
	switch (buttonEvent.button)
	{
	case SDL_CONTROLLER_BUTTON_START:
		button = Button::BUTTON_START;
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		button = Button::BUTTON_SELECT;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		button = Button::BUTTON_DLEFT;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		button = Button::BUTTON_DRIGHT;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		button = Button::BUTTON_DUP;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		button = Button::BUTTON_DDOWN;
		break;
	case SDL_CONTROLLER_BUTTON_A:
		button = Button::BUTTON_CROSS;
		break;
	case SDL_CONTROLLER_BUTTON_B:
		button = Button::BUTTON_CIRCLE;
		break;
	case SDL_CONTROLLER_BUTTON_X:
		button = Button::BUTTON_SQUARE;
		break;
	case SDL_CONTROLLER_BUTTON_Y:
		button = Button::BUTTON_TRIANGLE;
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		button = Button::BUTTON_L1;
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		button = Button::BUTTON_R1;
		break;
	default:
		// Unhandled button
		return;
	}

	pad->setButtonState(button, state);
}

static void updateControllerAxis(Profile* pad, SDL_JoyAxisEvent& joyAxisEvent)
{
	Button button;
	switch (joyAxisEvent.axis)
	{
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
		button = Button::BUTTON_L2;
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
		button = Button::BUTTON_R2;
		break;
	default:
		// Unhandled axis
		return;
	}

	ButtonState buttonState = ButtonState::BUTTON_STATE_PRESSED;
	if (joyAxisEvent.value < 0x4000)
		buttonState = ButtonState::BUTTON_STATE_RELEASED;

	pad->setButtonState(button, buttonState);
}

// Handle SDL events
static Action handleEvents(SDL_Event& event, Cpu& cpu)
{
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
			return Action::ACTION_QUIT;
		else if (event.type == SDL_KEYDOWN)
		{
			SDL_Keycode keyCode = event.key.keysym.sym;
			switch (keyCode)
			{
			case SDLK_ESCAPE:
				return Action::ACTION_QUIT;
			case SDLK_PAUSE:
				return Action::ACTION_DEBUG;
			}
			handleKeyboard(cpu.getPadProfiles()[0], keyCode, ButtonState::BUTTON_STATE_PRESSED);
		}
		else if (event.type == SDL_KEYUP)
		{
			SDL_Keycode keyCode = event.key.keysym.sym;
			handleKeyboard(cpu.getPadProfiles()[0], keyCode, ButtonState::BUTTON_STATE_RELEASED);
		}
		else if (event.type == SDL_CONTROLLERBUTTONDOWN)
		{
			SDL_ControllerButtonEvent buttonEvent = event.cbutton;
			handleController(cpu.getPadProfiles()[0], buttonEvent, ButtonState::BUTTON_STATE_PRESSED);
		}
		else if (event.type == SDL_CONTROLLERBUTTONUP)
		{
			SDL_ControllerButtonEvent buttonEvent = event.cbutton;
			handleController(cpu.getPadProfiles()[0], buttonEvent, ButtonState::BUTTON_STATE_RELEASED);
		}
		else if (event.type == SDL_CONTROLLERAXISMOTION)
		{
			SDL_JoyAxisEvent joyAxisEvent = event.jaxis;
			updateControllerAxis(cpu.getPadProfiles()[0], joyAxisEvent);
		}
	}

	return Action::ACTION_NONE;
}

int main(int argc, char** argv)
{
	ArgSetParser parser(argc, argv);
	const std::vector<std::string>& args = parser.args();

	// Should be two arguments at least
	if (args.size() < 2)
		printUsageAndExit(args[0].c_str());

	// Path to the BIOS
	std::string biosPath = args[1];

	bool discIsPresent                 = false;
	bool dumpInstructionsAndRegsToFile = false;
	bool runTesting                    = false;

	std::string discPath;

	// Parse command line arguments
	for (size_t i = 2; i < args.size(); ++i)
	{
		if (args[i] == "-h" || args[i] == "--help")
			printUsageAndExit(args[i].c_str());

		if (args[i] == "-disc" || args[i] == "--disc-bin-path")
		{
			discIsPresent = true;
			discPath = args[i + 1];
		}

		if (args[i] == "-dump" || args[i] == "--dump-instructions-registers")
			dumpInstructionsAndRegsToFile = true;

		if (args[i] == "-rt" || args[i] == "--run-testing")
			runTesting = true;
	}

	Bios bios;
	Bios::BiosState state = bios.loadBios(biosPath);

	switch (state)
	{
	case Bios::BIOS_STATE_INCORRECT_FILENAME:
		std::cout << "Can't find location of the bios " << biosPath << std::endl;
		return EXIT_FAILURE;
	case Bios::BIOS_STATE_INVALID_BIOS_SIZE:
		std::cout << "Invalid BIOS size " << biosPath << std::endl;
		return EXIT_FAILURE;
	}

	// Read bin disc format
	Disc::ResultDisc resultDisc(nullptr, Disc::DiscStatus::DISC_STATUS_OK);
	HardwareType videoStandard(HardwareType::HARDWARE_TYPE_NTSC);
	if (discIsPresent)
	{
		resultDisc = Disc::initializeFromPath(discPath);
		if (resultDisc.m_status == Disc::DiscStatus::DISC_STATUS_OK)
		{
			Region region = resultDisc.m_disc->getRegion();
			LOG("Disc region 0x" << std::hex << region);

			switch (region)
			{
			case Region::REGION_EUROPE:
				videoStandard = HardwareType::HARDWARE_TYPE_PAL;
				break;
			case Region::REGION_NORTH_AMERICA:
			case Region::REGION_JAPAN:
				videoStandard = HardwareType::HARDWARE_TYPE_NTSC;
				break;
			}
		}
	}

	Interconnect interconnect(bios, videoStandard, resultDisc.m_disc);
	Cpu cpu(interconnect);

	SDL_GameController* gameController = initializeSDL2Controllers();

	bool done = false;

	while (!done)
	{
		for (size_t i = 0; i < 10e6; ++i)
			cpu.runNextInstuction();

		SDL_Event event;
		switch (handleEvents(event, cpu))
		{
		case Action::ACTION_QUIT:
			done = true;
		}
	}

	//SDL_Quit();

	if (dumpInstructionsAndRegsToFile)
		generateDumpOutputFn(cpu);
	if (runTesting)
	{
		if (compareGoldenWithDump(cpu))
			LOG("Dump matches golden file");
		else
			LOG("Dump doesn't match to golden file");
	}
	return EXIT_SUCCESS;
}