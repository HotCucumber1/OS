#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

constexpr size_t BUFFER_SIZE = 64 * 1024;

void FlipAsciiCase(std::vector<char>& buffer);
std::string FlipFile(const std::string& filename);
void FlipFiles(const std::vector<std::string>& filenames, int maxProcesses = 100);
std::vector<std::string> ParseArgs(int argc, char* argv[]);
void AssertFilesAreSpecified(int argc);

int main(int argc, char* argv[])
{
	try
	{
		AssertFilesAreSpecified(argc);
		const auto files = ParseArgs(argc, argv);
		FlipFiles(files);
	}
	catch (const std::invalid_argument& exception)
	{
		std::cerr << exception.what() << std::endl;
		return 2;
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
		return 1;
	}
}

void AssertFilesAreSpecified(const int argc)
{
	if (argc < 2)
	{
		throw std::invalid_argument("Files not specified");
	}
}

void FlipAsciiCase(std::vector<char>& buffer)
{
	constexpr char addition = 'a' - 'A';

	for (auto& c : buffer)
	{
		if (c >= 'A' && c <= 'Z')
		{
			c += addition;
		}
		else if (c >= 'a' && c <= 'z')
		{
			c -= addition;
		}
	}
}

std::string FlipFile(const std::string& filename)
{
	const auto outputFilename = filename + ".out";

	std::ifstream inputFile(filename, std::ios::binary);
	std::ofstream outputFile(outputFilename, std::ios::binary);

	if (!inputFile.is_open() || !outputFile.is_open())
	{
		throw std::runtime_error("Failed to open file " + filename);
	}

	std::vector<char> buffer(BUFFER_SIZE);
	while (inputFile)
	{
		inputFile.read(buffer.data(), BUFFER_SIZE);
		auto bytesRead = inputFile.gcount();

		if (bytesRead == 0)
		{
			break;
		}
		FlipAsciiCase(buffer);

		outputFile.write(buffer.data(), bytesRead);
		if (outputFile.bad())
		{
			throw std::runtime_error("Failed to write to file " + filename);
		}
	}
	return outputFilename;
}

void FlipFiles(
	const std::vector<std::string>& filenames,
	const int maxProcesses)
{
	if (maxProcesses <= 0)
	{
		throw std::invalid_argument("maxProcesses must be positive");
	}

	std::vector<pid_t> activeProcesses;
	size_t currentIndex = 0;

	while (currentIndex < filenames.size() || !activeProcesses.empty())
	{
		while (activeProcesses.size() < maxProcesses && currentIndex < filenames.size())
		{
			const pid_t pid = fork();
			if (pid < 0)
			{
				std::cerr << "Fork failed: " << std::strerror(errno) << std::endl;
				throw std::runtime_error("Fork failed");
			}

			if (pid == 0)
			{
				std::cout << "Process " << getpid() << " is processing " << filenames[currentIndex] << std::endl;
				const auto outFilename = FlipFile(filenames[currentIndex]);
				std::cout << "Process " << getpid() << " has finished writing to " << outFilename << std::endl;
				_exit(0);
			}
			activeProcesses.push_back(pid);
			std::cout << "Started process " << pid << " for file " << filenames[currentIndex] << std::endl;
			currentIndex++;
		}

		if (!activeProcesses.empty())
		{
			int status;
			const auto childPid = waitpid(-1, &status, 0);

			if (childPid == -1)
			{
				if (errno == EINTR)
				{
					std::cout << "waitpid() was interrupted (EINTR)" << std::endl;
					continue;
				}
				if (errno == ECHILD)
				{
					std::cout << "No more children" << std::endl;
					break;
				}
				std::cerr << "waitpid failed: " << std::strerror(errno) << std::endl;
				break;
			}
			auto it = std::ranges::find(activeProcesses, childPid);
			if (it != activeProcesses.end())
			{
				activeProcesses.erase(it);
				std::cout << "Child process " << childPid << " finished" << std::endl;
			}
		}
	}
}

std::vector<std::string> ParseArgs(const int argc, char* argv[])
{
	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i)
	{
		args.emplace_back(argv[i]);
	}
	return args;
}