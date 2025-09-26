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

	if (pid == 0)
	{
//		std::cout << "Child process: ";
//		std::cout << "my pid - " << getpid() << std::endl;
		std::cout << "Hello, I am a child process" << std::endl;
		_exit(0);
	}
	else
	{
		std::cout << "Parent process: ";
		std::cout << "Child pid: " << pid << std::endl;

		sleep(1);

		pid_t userPid;
		std::cout << "Enter your pid: ";
		std::cin >> userPid;

		int status;
		pid_t result = waitpid(userPid, &status, 0);
		while (result < 0)
		{
			std::cout << "Wrong pid, try again!" << std::endl;
			std::cout << "Enter your pid: ";
			std::cin >> userPid;
			result = waitpid(userPid, &status, 0);
		}
	}
}
