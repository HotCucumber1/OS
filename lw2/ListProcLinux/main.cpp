#include "ProcessLister.h"
#include <iostream>


int main()
{
	try
	{
		std::cout << "=== Linux process information ===" << std::endl;

		ProcessLister lister;
		if (!lister.GetProcesses())
		{
			std::cerr << "Failed to fetch processes info" << std::endl;
			return 1;
		}
		lister.PrintProcessesInfo();
		return 0;
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
		return 1;
	}
}
