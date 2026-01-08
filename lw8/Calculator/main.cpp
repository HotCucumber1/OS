#include "Client/Client.h"
#include "Server/Server.h"
#include <iostream>


int main(const int argc, char* argv[])
{
	try
	{
		switch (argc)
		{
		case 2:
			RunServer(std::stoi(argv[1]));
			break;
		case 3:
			RunClient(argv[1], std::stoi(argv[2]));
			break;
		default:
			std::cout << "Server Usage: calc <PORT>" << std::endl
					  << "Client Usage: calc <ADDRESS> <PORT>" << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}
}

