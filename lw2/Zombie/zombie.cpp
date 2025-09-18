#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/wait.h>

int main()
{
	auto pid = fork();

	if (pid < 0)
	{
		std::cerr << "Fork failed: " << std::strerror(errno) << std::endl;
		return 1;
	}
}
