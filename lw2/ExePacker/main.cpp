#include "DataPacker.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

constexpr int SIGNATURE_SIZE = 8;
constexpr int MAX_PATH_SIZE = 1024;
constexpr char SIGNATURE[SIGNATURE_SIZE] = { 'S', 'F', 'X', 'P', 'A', 'C', 'K', '!' };

struct PayloadInfo
{
	size_t originSize;
	size_t payloadSize;
	char signature[SIGNATURE_SIZE];
};

struct SelfFileInfo
{
	bool hasPayload;
	std::string selfPath;
	PayloadInfo payloadInfo;
};

void Pack(
	const std::string& selfPath,
	const std::string& inputFilename,
	const std::string& outputFilename);
void Extract(
	const std::string& selfPath,
	char* argv[],
	const PayloadInfo& payloadInfo);
SelfFileInfo GetSelfFileInfo();
void AssertArgumentCount(int argc);
std::vector<char> ReadFile(const std::string& path);
std::vector<char> GetPayloadData(
	const std::string& selfPath,
	const PayloadInfo& payloadInfo);
void AssertTempFileCreated(int fileDescriptor);
void ForkProcess(char* tempFile, char* argv[]);

int main(int argc, char* argv[])
{
	try
	{
		auto [hasPayload, selfPath, payloadInfo] = GetSelfFileInfo();
		if (hasPayload)
		{
			std::cout << "It is a process with payload" << std::endl;
			Extract(selfPath, argv, payloadInfo);
		}
		else
		{
			AssertArgumentCount(argc);
			std::cout << "It is an origin process (without payload)" << std::endl;
			const std::string inputExeFilename = argv[1];
			const std::string outputSelfpackFilename = argv[2];
			Pack(selfPath, inputExeFilename, outputSelfpackFilename);
		}
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
		return 1;
	}
}

void AssertInputIsOpen(const std::ifstream& inputFile, const std::string& fileName)
{
	if (!inputFile)
	{
		throw std::runtime_error("Failed to open in-file: " + fileName);
	}
}

void AssertOutputIsOpen(const std::ofstream& outputFile, const std::string& fileName)
{
	if (!outputFile)
	{
		throw std::runtime_error("Failed to open out-file: " + fileName);
	}
}

SelfFileInfo GetSelfFileInfo()
{
	char selfPathBuf[MAX_PATH_SIZE];
	const auto len = readlink("/proc/self/exe", selfPathBuf, MAX_PATH_SIZE);
	if (len == -1)
	{
		throw std::runtime_error("Failed to define path to source file");
	}

	const std::string selfPath(selfPathBuf, len);
	std::ifstream selfFile(selfPath, std::ios::binary | std::ios::ate);
	AssertInputIsOpen(selfFile, selfPath);

	PayloadInfo payloadInfo{};
	bool payloadFound = false;

	if (selfFile.tellg() > sizeof(PayloadInfo))
	{
		selfFile.seekg(-sizeof(PayloadInfo), std::ios::end);
		selfFile.read(reinterpret_cast<char*>(&payloadInfo), sizeof(PayloadInfo));

		if (memcmp(payloadInfo.signature, SIGNATURE, sizeof(SIGNATURE)) == 0)
		{
			payloadFound = true;
		}
	}
	selfFile.close();

	return {
		payloadFound,
		selfPath,
		payloadInfo
	};
}

void Pack(
	const std::string& selfPath,
	const std::string& inputFilename,
	const std::string& outputFilename)
{
	auto inFileData = ReadFile(inputFilename);
	if (inFileData.empty())
	{
		throw std::runtime_error("Failed to read file or it is empty");
	}
	auto payloadData = DataPacker::PackData(inFileData);

	PayloadInfo payloadInfo{};
	payloadInfo.originSize = inFileData.size();
	payloadInfo.payloadSize = payloadData.size();
	memcpy(payloadInfo.signature, SIGNATURE, sizeof(SIGNATURE));

	std::ifstream selfFile(selfPath, std::ios::binary);
	AssertInputIsOpen(selfFile, selfPath);

	std::ofstream outputFile(outputFilename, std::ios::binary);
	AssertOutputIsOpen(outputFile, outputFilename);

	outputFile << selfFile.rdbuf();
	selfFile.close();

	outputFile.write(payloadData.data(), payloadData.size());
	outputFile.write(reinterpret_cast<const char*>(&payloadInfo), sizeof(payloadInfo));
	AssertOutputIsOpen(outputFile, outputFilename);
	outputFile.close();

	if (chmod(outputFilename.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
	{
		throw std::runtime_error("Fail to add exe-rights to file " + outputFilename);
	}
}

void Extract(
	const std::string& selfPath,
	char* argv[],
	const PayloadInfo& payloadInfo)
{
	char tempFile[] = "/tmp/exe-pack-XXXXXX";

	const auto payloadData = GetPayloadData(selfPath, payloadInfo);
	const auto extractedData = DataPacker::UnpackData(payloadData, payloadInfo.originSize);

	const auto descriptor = mkstemp(tempFile);
	AssertTempFileCreated(descriptor);

	auto bytesWritten = write(descriptor, extractedData.data(), extractedData.size());
	if (bytesWritten != extractedData.size())
	{
		close(descriptor);
		unlink(tempFile);
		throw std::runtime_error("Failed to write to temp file");
	}
	if (fchmod(descriptor, S_IRWXU) != 0)
	{
		throw std::runtime_error("Failed to change temp file rights");
	}
	close(descriptor);
	ForkProcess(tempFile, argv);
}

void ForkProcess(char* tempFile, char* argv[])
{
	const auto pid = fork();
	if (pid == -1)
	{
		unlink(tempFile);
		throw std::runtime_error("Failed to fork");
	}

	if (pid == 0)
	{
		argv[0] = tempFile;
		execv(tempFile, argv);

		std::cerr << "Failed to exec temp file" << std::endl;
		_exit(127);
	}

	int status;
	if (waitpid(pid, &status, 0) == -1)
	{
		std::cerr << "Waitpid failed" << std::endl;
	}
	unlink(tempFile);
}

std::vector<char> GetPayloadData(
	const std::string& selfPath,
	const PayloadInfo& payloadInfo)
{
	std::ifstream selfFile(selfPath, std::ios::binary);
	AssertInputIsOpen(selfFile, selfPath);

	selfFile.seekg(-(sizeof(PayloadInfo) + payloadInfo.payloadSize), std::ios::end);
	std::vector<char> payloadData(payloadInfo.payloadSize);
	selfFile.read(payloadData.data(), payloadInfo.payloadSize);

	selfFile.close();
	return payloadData;
}

std::vector<char> ReadFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		throw std::runtime_error("Failed to open file " + path);
	}

	return {
		(std::istreambuf_iterator(file)),
		std::istreambuf_iterator<char>()
	};
}

std::vector<char> ZipData(const std::vector<char>& srcData)
{
	return srcData;
}

std::vector<char> UnzipData(const std::vector<char>& srcData)
{
	return srcData;
}

void AssertArgumentCount(const int argc)
{
	constexpr int argumentExpected = 3;
	if (argc != argumentExpected)
	{
		throw std::invalid_argument("Need to specify only <inputFile> and <outputFile>");
	}
}

void AssertTempFileCreated(const int fileDescriptor)
{
	if (fileDescriptor == -1)
	{
		throw std::runtime_error("Failed to create temp file");
	}
}