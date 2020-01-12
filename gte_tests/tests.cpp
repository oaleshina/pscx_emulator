#include "pscx_gte.h"

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>

using RegMap = std::unordered_map<uint8_t, uint32_t>;

inline void CHECK(std::string testCase, bool result)
{
	if (result)
	{
		std::cout << "Test case passed: ";
	}
	else
	{
		std::cout << "Test case failed: ";
	}
	std::cout << testCase << std::endl;
}

// GTE register config: slice of couples (register_offset, register_value).
// Missing registers are set to 0
struct Config
{
	Gte* make_gte()
	{
		Gte* gte = new Gte;
		for (auto& control : controls)
		{
			gte->setControl(static_cast<uint32_t>(control.first), control.second);
		}

		for (auto& reg : data)
		{
			if (reg.first == 15)
			{
				continue;
			}
			if (reg.first == 28)
			{
				continue;
			}
			if (reg.first == 29)
			{
				// Read only register
				continue;
			}
			gte->setData(static_cast<uint32_t>(reg.first), reg.second);
		}
		return gte;
	}

	uint32_t validate(const Gte* gte)
	{
		uint32_t errorCount = 0UL;
		for (auto& control : controls)
		{
			uint32_t value = gte->getControl(static_cast<uint32_t>(control.first));
			if (value != control.second)
			{
				std::cout << "Control register " << std::hex << control.first <<
					" expected " << control.second << " got " << value << std::endl;
				errorCount++;
			}
		}

		for (auto& reg : data)
		{
			uint32_t value = gte->getData(static_cast<uint32_t>(reg.first));
			if (value != reg.second)
			{
				std::cout << "Data register " << std::hex << reg.first <<
					" expected " << reg.second << " got " << value << std::endl;
				errorCount++;
			}
		}

		return errorCount;
	}

	// Control register
	RegMap controls;
	// Data registers
	RegMap data;
};

struct Test
{
	// Test description
	std::string desc;
	// Initial GTE configuration
	Config initial;
	// GTE command being executed
	uint32_t command;
	// GTE configuration post-command
	Config result;
};

static bool gte_lzcr()
{
	std::unordered_map<uint32_t, uint32_t> expected =
	{
		{ 0x00000000, 32 },
		{ 0xffffffff, 32 },
		{ 0x00000001, 31 },
		{ 0x80000000, 1 },
		{ 0x7fffffff, 1 },
		{ 0xdeadbeef, 2 },
		{ 0x000c0ffe, 12 },
		{ 0xfffc0ffe, 14 }
	};

	Gte gte;
	for (auto exp : expected)
	{
		gte.setData(30, exp.first);
		if (gte.getData(31) != exp.second)
		{
			return false;
		}
	}
	return true;
}

std::vector<Test> TESTS = 
{
	// Test cmdRotateTranslatePerspectiveTransform()
	{
		"GTE_RTPT, lm=0, cv=0, v=0, mx=0, sf=1", // desc
		RegMap // initial.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // initial.data
		{
			{ 0, 0x00e70119 },
			{ 1, 0xfffffe65 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 31, 0x00000020 }
		},
		0x00080030, // command
		RegMap // result.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 },
			{ 31, 0x00001000 }
		},
		RegMap // result.data
		{
			{ 0, 0x00e70119 },
			{ 1, 0xfffffe65 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 8, 0x00001000 },
			{ 9, 0x0000012b },
			{ 10, 0xfffffff0 },
			{ 11, 0x000015d9 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 24, 0x0106e038 },
			{ 25, 0x0000012b },
			{ 26, 0xfffffff0 },
			{ 27, 0x000015d9 },
			{ 28, 0x00007c02 },
			{ 29, 0x00007c02 },
			{ 31, 0x00000020 }
		}
	},
	// Test cmdNormalClip()
	{
		"GTE_NCLIP, lm=0, cv=0, v=0, mx=0, sf=0", // desc
		RegMap // initial.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 },
			{ 31, 0x00001000 }
		},
		RegMap // initial.data
		{
			{ 0, 0x00e70119 },
			{ 1, 0xfffffe65 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 8, 0x00001000 },
			{ 9, 0x0000012b },
			{ 10, 0xfffffff0 },
			{ 11, 0x000015d9 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 24, 0x0106e038 },
			{ 25, 0x0000012b },
			{ 26, 0xfffffff0 },
			{ 27, 0x000015d9 },
			{ 28, 0x00007c02 },
			{ 29, 0x00007c02 },
			{ 31, 0x00000020 }
		},
		0x00000006, // command
		RegMap // result.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // result.data
		{
			{ 0, 0x00e70119 },
			{ 1, 0xfffffe65 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 8, 0x00001000 },
			{ 9, 0x0000012b },
			{ 10, 0xfffffff0 },
			{ 11, 0x000015d9 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 24, 0x0000004d },
			{ 25, 0x0000012b },
			{ 26, 0xfffffff0 },
			{ 27, 0x000015d9 },
			{ 28, 0x00007c02 },
			{ 29, 0x00007c02 },
			{ 31, 0x00000020 }
		}
	},
	// Test cmdAverageSingleZ3()
	{
		"GTE_AVSZ3, lm=0, cv=0, v=0, mx=0, sf=1", // initial.controls
		RegMap // initial.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // initial.data
		{
			{ 0, 0x00e70119 },
			{ 1, 0xfffffe65 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 8, 0x00001000 },
			{ 9, 0x0000012b },
			{ 10, 0xfffffff0 },
			{ 11, 0x000015d9 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 24, 0x0000004d },
			{ 25, 0x0000012b },
			{ 26, 0xfffffff0 },
			{ 27, 0x000015d9 },
			{ 28, 0x00007c02 },
			{ 29, 0x00007c02 }
		},
		0x0008002d, // command
		RegMap // result.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // result.data
		{
			{ 0, 0x00e70119 },
			{ 1, 0xfffffe65 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 7, 0x00000572 },
			{ 8, 0x00001000 },
			{ 9, 0x0000012b },
			{ 10, 0xfffffff0 },
			{ 11, 0x000015d9 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 24, 0x00572786 },
			{ 25, 0x0000012b },
			{ 26, 0xfffffff0 },
			{ 27, 0x000015d9 },
			{ 28, 0x00007c02 },
			{ 29, 0x00007c02 },
			{ 31, 0x00000020 }
		}
	},
	// Test cmdNormalColorDepthSingleVector()
	{
		"GTE_NCDS, lm=1, cv=0, v=0, mx=0, sf=1", // desc
		RegMap // initial.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // initial.data
		{
			{ 0, 0x00000b50 },
			{ 1, 0xfffff4b0 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 6, 0x2094a539 },
			{ 7, 0x00000572 },
			{ 8, 0x00001000 },
			{ 9, 0x0000012b },
			{ 10, 0xfffffff0 },
			{ 11, 0x000015d9 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 24, 0x00572786 },
			{ 25, 0x0000012b },
			{ 26, 0xfffffff0 },
			{ 27, 0x000015d9 },
			{ 28, 0x00007c02 },
			{ 29, 0x00007c02 },
			{ 31, 0x00000020 }
		},
		0x00080413, // command
		RegMap // result.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 },
			{ 31, 0x81f00000 }
		},
		RegMap // result.data
		{
			{ 0, 0x00000b50 },
			{ 1, 0xfffff4b0 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 6, 0x2094a539 },
			{ 7, 0x00000572 },
			{ 8, 0x00001000 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 22, 0x20000000 },
			{ 24, 0x00572786 },
			{ 25, 0xffffffff },
			{ 26, 0xffffffff },
			{ 31, 0x00000020 }
		}
	},
	// Test cmdDepthQueueSingle()
	{
		"GTE_DCPS, lm=0, cv=0, v=0, mx=0, sf=1", // desc
		RegMap // initial.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // initial.data
		{
			{ 0, 0x00000b50 },
			{ 1, 0xfffff4b0 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 6, 0x2094a539 },
			{ 7, 0x00000572 },
			{ 8, 0x00001000 },
			{ 9, 0x0000012b },
			{ 10, 0xfffffff0 },
			{ 11, 0x000015d9 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 24, 0x00572786 },
			{ 25, 0x0000012b },
			{ 26, 0xfffffff0 },
			{ 27, 0x000015d9 },
			{ 28, 0x00007c02 },
			{ 29, 0x00007c02 },
			{ 31, 0x00000020 }
		},
		0x00080010, // command
		RegMap // result.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // result.data
		{
			{ 0, 0x00000b50 },
			{ 1, 0xfffff4b0 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 6, 0x2094a539 },
			{ 7, 0x00000572 },
			{ 8, 0x00001000 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 22, 0x20000000 },
			{ 24, 0x00572786 },
			{ 31, 0x00000020 }
		}
	},
	// Test cmdRotateTranslatePerspectiveTransformSingle(config)
	{
		"GTE_RTPS, lm=0, cv=0, v=0, mx=0, sf=1", // desc
		RegMap // initial.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // initial.data
		{
			{ 0, 0x00000b50 },
			{ 1, 0xfffff4b0 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 6, 0x2094a539 },
			{ 8, 0x00001000 },
			{ 31, 0x00000020 }
		},
		0x00080001, // command
		RegMap // result.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 },
			{ 31, 0x80004000 }
		},
		RegMap // result.data
		{
			{ 0, 0x00000b50 },
			{ 1, 0xfffff4b0 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 6, 0x2094a539 },
			{ 8, 0x00000e08 },
			{ 9, 0x00000bd1 },
			{ 10, 0x000002dc },
			{ 11, 0x00000d12 },
			{ 14, 0x01d003ff },
			{ 15, 0x01d003ff },
			{ 19, 0x00000d12 },
			{ 24, 0x00e08388 },
			{ 25, 0x00000bd1 },
			{ 26, 0x000002dc },
			{ 27, 0x00000d12 },
			{ 28, 0x000068b7 },
			{ 29, 0x000068b7 },
			{ 31, 0x00000020 }
		}
	},
	// Test cmdNormalColorColorTriple(config)
	{
		"GTE_NCCT, lm=0, cv=0, v=0, mx=0, sf=1", // desc
		RegMap // initial.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 }
		},
		RegMap // initial.data
		{
			{ 0, 0x00000b50 },
			{ 1, 0xfffff4b0 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 6, 0x2094a539 },
			{ 7, 0x00000572 },
			{ 8, 0x00001000 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 24, 0x00572786 },
			{ 31, 0x00000020 }
		},
		0x0008003f, // command
		RegMap // result.controls
		{
			{ 0, 0x00000ffb },
			{ 1, 0xffb7ff44 },
			{ 2, 0xf9ca0ebc },
			{ 3, 0x063700ad },
			{ 4, 0x00000eb7 },
			{ 6, 0xfffffeac },
			{ 7, 0x00001700 },
			{ 9, 0x00000fa0 },
			{ 10, 0x0000f060 },
			{ 11, 0x0000f060 },
			{ 13, 0x00000640 },
			{ 14, 0x00000640 },
			{ 15, 0x00000640 },
			{ 16, 0x0bb80fa0 },
			{ 17, 0x0fa00fa0 },
			{ 18, 0x0fa00bb8 },
			{ 19, 0x0bb80fa0 },
			{ 20, 0x00000fa0 },
			{ 24, 0x01400000 },
			{ 25, 0x00f00000 },
			{ 26, 0x00000400 },
			{ 27, 0xfffffec8 },
			{ 28, 0x01400000 },
			{ 29, 0x00000155 },
			{ 30, 0x00000100 },
			{ 31, 0x00380000 }
		},
		RegMap // result.data
		{
			{ 0, 0x00000b50 },
			{ 1, 0xfffff4b0 },
			{ 2, 0x00e700d5 },
			{ 3, 0xfffffe21 },
			{ 4, 0x00b90119 },
			{ 5, 0xfffffe65 },
			{ 6, 0x2094a539 },
			{ 7, 0x00000572 },
			{ 8, 0x00001000 },
			{ 9, 0x000000b3 },
			{ 10, 0x00000207 },
			{ 11, 0x000001d1 },
			{ 12, 0x00f40176 },
			{ 13, 0x00f9016b },
			{ 14, 0x00ed0176 },
			{ 15, 0x00ed0176 },
			{ 17, 0x000015eb },
			{ 18, 0x000015aa },
			{ 19, 0x000015d9 },
			{ 20, 0x20000000 },
			{ 21, 0x201b1f0a },
			{ 22, 0x201d200b },
			{ 24, 0x00572786 },
			{ 25, 0x000000b3 },
			{ 26, 0x00000207 },
			{ 27, 0x000001d1 },
			{ 28, 0x00000c81 },
			{ 29, 0x00000c81 },
			{ 31, 0x00000020 }
		}
	}
};

static uint32_t gte_ops()
{
	uint32_t returnCode(0x0);
	for (auto& test : TESTS)
	{
		std::cout << "Test: " << test.desc << std::endl;
		std::cout << "Command: " << test.command << std::endl;

		std::unique_ptr<Gte> gte(test.initial.make_gte());
		gte->command(test.command);
		returnCode = test.result.validate(gte.get());

		if (returnCode > 0x0)
			break;
	}

	return returnCode;
}

static void test_divider() 
{
	// Tested against mednafen's "Divide" function's output. We only
	// reach the division if numerator < (divisor * 2).
	CHECK("divide(0, 1)", divide(0, 1) == 0);
	CHECK("divide(0, 1234)", divide(0, 1234) == 0);
	CHECK("divide(1, 1)", divide(1, 1) == 0x10000);
	CHECK("divide(2, 2)", divide(2, 2) == 0x10000);
	CHECK("divide(0xffff, 0xffff)", divide(0xffff, 0xffff) == 0xffff);
	CHECK("divide(0xffff, 0xfffe)", divide(0xffff, 0xfffe) == 0x10000);
	CHECK("divide(1, 2)", divide(1, 2) == 0x8000);
	CHECK("divide(1, 3)", divide(1, 3) == 0x5555);
	CHECK("divide(5, 6)", divide(5, 6) == 0xd555);
	CHECK("divide(1, 4)", divide(1, 4) == 0x4000);
	CHECK("divide(10, 40)", divide(10, 40) == 0x4000);
	CHECK("divide(0xf00, 0xbeef)", divide(0xf00, 0xbeef) == 0x141d);
	CHECK("divide(9876, 8765)", divide(9876, 8765) == 0x12072);
	CHECK("divide(200, 10000)", divide(200, 10000) == 0x51f);
	CHECK("divide(0xffff, 0x8000)", divide(0xffff, 0x8000) == 0x1fffe);
	CHECK("divide(0xe5d7, 0x72ec)", divide(0xe5d7, 0x72ec) == 0x1ffff);
}

int main()
{
	CHECK("Calculate leading zeroes", gte_lzcr() == true);
	CHECK("Test commands", gte_ops() == 0x0);
	test_divider();
	return EXIT_SUCCESS;
}
