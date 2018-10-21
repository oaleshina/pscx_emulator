#pragma once

#include <iostream>

#if _DEBUG
#define LOG(msg) \
	std::cout << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl 
#else
#define LOG(msg)
#endif

#define WARN(msg) \
	std::cerr << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl 

