#include "pscx_bios.h"
#include "pscx_cpu.h"
#include "pscx_interconnect.h"

#include <iostream>

int main()
{
	std::string biosPath = "roms/SCPH1001.BIN";

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

	Interconnect interconnect(bios);
	Cpu cpu(interconnect);

	while (cpu.runNextInstuction() != Cpu::INSTRUCTION_TYPE_UNKNOWN);

	return EXIT_SUCCESS;
}