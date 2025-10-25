#include "Controller/Controller.h"

#include <chrono>
#include <iostream>

int main()
{
	try
	{
		Controller controller(std::cin, std::cout, 16);
		controller.Run();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return -1;
	}
}

// add_dir_recursive /home/dmitriy.rybakov/projects/crm/crm-app/lib
// find lead deal qualify