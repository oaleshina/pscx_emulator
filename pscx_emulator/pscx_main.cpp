#include "pscx_bios.h"
#include "pscx_cpu.h"
#include "pscx_interconnect.h"

#include <iostream>
#include <fstream>

#include <vector>
#include <string>

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
	<< "Usage  : " << argv0 << " <Path to BIOS BIN> [options]\n"
	<< "App options:\n"
	<< "  -h    | --help                        Print this usage message\n"
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

int main(int argc, char** argv)
{
	ArgSetParser parser(argc, argv);
	const std::vector<std::string>& args = parser.args();

	// Should be two arguments at least
	if (args.size() < 2)
		printUsageAndExit(args[0].c_str());

	// Path to the BIOS
	std::string biosPath = args[1];

	bool dumpInstructionsAndRegsToFile = false;
	bool runTesting                    = false;

	// Parse command line arguments
	for (size_t i = 2; i < args.size(); ++i)
	{
		if (args[i] == "-h" || args[i] == "--help")
			printUsageAndExit(args[i].c_str());

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

	Interconnect interconnect(bios);
	Cpu cpu(interconnect);

	while (cpu.runNextInstuction() != Cpu::INSTRUCTION_TYPE_UNKNOWN);

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