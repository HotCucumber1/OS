#include "FlashImgReader.h"
#include <iostream>

void AssertArgumentsCount(int argc);

int main(int argc, char* argv[])
{
	try
	{
		AssertArgumentsCount(argc);

		const std::string imagePath = argv[1];
		const std::string searchPath = argv[2];

		FlashImgReader reader(imagePath, std::cout);
		reader.PrintBase();
		std::cout << std::endl;
		reader.PrintInfo(searchPath);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void AssertArgumentsCount(const int argc)
{
	if (argc != 3)
	{
		throw std::runtime_error("Usage: <fat32.img> <path/in/img>");
	}
}