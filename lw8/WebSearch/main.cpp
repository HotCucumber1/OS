#include <iostream>

int main(const int argc, char* argv[])
{
	try
	{
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}
}

