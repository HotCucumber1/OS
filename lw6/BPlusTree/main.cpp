#include "BPlusTree.h"

#include <algorithm>
#include <memory>
#include <sstream>

void PrintHelp();
int TreeApp(const std::string& filepath);

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			PrintHelp();
			return 1;
		}

		const std::string filepath = argv[1];
		if (TreeApp(filepath) == 1)
		{
			return 1;
		}
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}

void PrintHelp()
{
	std::cout << "Usage: BPlusTree <filepath>" << std::endl;
	std::cout << "Commands:" << std::endl;
	std::cout << "  GET <key>          -> Prints value for key or 'NOT FOUND'" << std::endl;
	std::cout << "  PUT <key> <value>  -> Inserts or updates key-value pair" << std::endl;
	std::cout << "  DEL <key>          -> Deletes key (if exists)" << std::endl;
	std::cout << "  STATS              -> Prints tree parameters" << std::endl;
	std::cout << "  QUIT               -> Exit and flush data" << std::endl;
}

int TreeApp(const std::string& filepath)
{
	std::unique_ptr<BPlusTree> tree = nullptr;

	try
	{
		tree = std::make_unique<BPlusTree>(filepath, std::cout);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error initializing B+ Tree: " << e.what() << std::endl;
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unknown error initializing B+ Tree." << std::endl;
		return 1;
	}

	std::cout << "B+ Tree loaded successfully from: " << filepath << std::endl;
	std::cout << "Enter command (GET/PUT/DEL/STATS/QUIT):" << std::endl;

	std::string line;
	while (std::getline(std::cin, line))
	{
		std::stringstream ss(line);
		std::string command;
		ss >> command;

		std::ranges::transform(command, command.begin(), ::toupper);

		if (command == "QUIT")
		{
			break;
		}
		if (command == "STATS")
		{
			tree->Stats();
		}
		else if (command == "GET")
		{
			KEY key;
			if (ss >> key)
			{
				tree->Get(key);
			}
			else
			{
				std::cout << "Invalid GET command format. Use: GET <key>" << std::endl;
			}
		}
		else if (command == "PUT")
		{
			KEY key;
			std::string value;
			ss >> key;
			if (std::getline(ss >> std::ws, value))
			{
				if (value.empty())
				{
					std::cout << "Invalid PUT command format. Use: PUT <key> <value>" << std::endl;
				}
				else
				{
					tree->Put(key, value);
				}
			}
			else
			{
				std::cout << "Invalid PUT command format. Use: PUT <key> <value>" << std::endl;
			}
		}
		else if (command == "DEL")
		{
			KEY key;
			if (ss >> key)
			{
				tree->Delete(key);
			}
			else
			{
				std::cout << "Invalid DEL command format. Use: DEL <key>" << std::endl;
			}
		}
		else
		{
			std::cout << "Unknown command: " << command << std::endl;
			PrintHelp();
		}
	}
	return 0;
}
