// Test00.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "MZHandler.h"

#include <time.h>
#include <string>
#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
	srand((unsigned int)time(NULL));

	bool bGenerateMZs = 0;

	MZHandler* handler = new MZHandler();
	if (handler->Go((bool)bGenerateMZs) == 0)
		return 0;

	std::cout << "Hit <Enter> to exit\n";
	std::string input;
	std::getline(std::cin, input);
}


