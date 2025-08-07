#include "SystemClass.h"

#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	AllocConsole();
	FILE* fpOut;
	freopen_s(&fpOut, "CONOUT$", "w", stdout);
	FILE* fpErr;
	freopen_s(&fpErr, "CONOUT$", "w", stderr);
	FILE* fpIn;
	freopen_s(&fpIn, "CONIN$", "r", stdin);

	std::cout << "Console attached!" << std::endl;
	
	SystemClass* System;
	bool Result;

	System = new SystemClass();

	Result = System->Initialise();
	if (Result)
	{
		System->Run();
	}

	System->Shutdown();
	delete System;
	System = 0;

	std::cout << "App finished! Press ENTER to quit." << std::endl;
	std::cin.get();

	return 0;
}