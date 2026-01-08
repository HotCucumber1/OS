#include <iostream>

int main()
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