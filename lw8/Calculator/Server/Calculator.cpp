#include "Calculator.h"
#include <sstream>

int ProcessCommand(const std::string& command)
{
	std::stringstream m_input(command);
	char operation;
	if (!(m_input >> operation) || (operation != '+' && operation != '-'))
	{
		throw std::runtime_error("ERROR Invalid operation");
	}

	int res = 0;
	int curr;
	bool first = true;
	int counter = 0;

	while (m_input >> curr)
	{
		if (first)
		{
			res = curr;
			first = false;
			continue;
		}
		if (operation == '+')
		{
			res += curr;
		}
		else
		{
			res -=curr;
		}
		counter++;
	}

	if (counter == 0)
	{
		throw std::runtime_error("ERROR No input data");
	}
	if (m_input.fail() && !m_input.eof())
	{
		throw std::runtime_error("ERROR Wrong input format");
	}
	return res;
}
