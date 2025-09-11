#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <windows.h>
#include <lmcons.h>
#include <VersionHelpers.h>

#pragma comment(lib, "secur32.lib")

constexpr int BYTES_IN_KB = 1024;

void PrintWindowsVersion();
void PrintMemoryInfo();
void PrintCoreInfo();
void PrintComputerName();
void PrintUserName();
void PrintDrivesInfo();


int main()
{
	try
	{
		PrintWindowsVersion();
		PrintCoreInfo();
		PrintMemoryInfo();
		PrintComputerName();
		PrintUserName();
		PrintDrivesInfo();
	}
	catch (const std::exception& exception)
	{
		std::cout << exception.what() << std::endl;
		return 1;
	}
}


std::string GetWindowsVersion()
{
    if (IsWindows10OrGreater())
    {
        return "Windows 10 or more";
    }
    if (IsWindows8Point1OrGreater())
    {
        return "Windows 8.1";
    }
    if (IsWindows8OrGreater())
    {
        return "Windows 8";
    }
    if (IsWindows7OrGreater())
    {
        return "Windows 7";
    }
    if (IsWindowsVistaOrGreater())
    {
        return "Windows Vista";
    }
    if (IsWindowsXPOrGreater())
    {
        return "Windows XP";
    }
    return "Undefined Windows version";
}

void PrintMemoryInfo(
	const unsigned long long availableMemory,
	const unsigned long long totalMemory,
	const std::string& infoTitle)
{
	std::cout << std::fixed << std::setprecision(2);

	std::cout << infoTitle << ": " << availableMemory << "mb / " << totalMemory << "mb ("
			<< static_cast<double>(availableMemory) / totalMemory * 100.0 << "%)" << std::endl;
}


void PrintWindowsVersion()
{
    std::cout << "OS: " << GetWindowsVersion() << std::endl;
}

void PrintMemoryInfo()
{
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex))
	{
		const auto availablePhysMemory = statex.ullAvailPhys / (BYTES_IN_KB * BYTES_IN_KB);
		const auto totalPhysMemory = statex.ullTotalPhys / (BYTES_IN_KB * BYTES_IN_KB);

		const auto availableVirtualMemory = statex.ullAvailVirtual / (BYTES_IN_KB * BYTES_IN_KB);
		const auto totalVirtualMemory = statex.ullTotalVirtual / (BYTES_IN_KB * BYTES_IN_KB);

		const auto availablePageFile = statex.ullAvailPageFile / (BYTES_IN_KB * BYTES_IN_KB);
		const auto totalPageFile = statex.ullTotalPageFile / (BYTES_IN_KB * BYTES_IN_KB);

		PrintMemoryInfo(availablePhysMemory, totalPhysMemory, "RAM");
		PrintMemoryInfo(availableVirtualMemory, totalVirtualMemory, "Virtual memory");
		PrintMemoryInfo(availablePageFile, totalPageFile, "Pagefile");
	}
}

std::string GetProcessorArchitectureStr(const WORD architecture)
{
	switch (architecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		return "x64";
	case PROCESSOR_ARCHITECTURE_ARM:
		return "ARM";
	case PROCESSOR_ARCHITECTURE_ARM64:
		return "ARM64";
	case PROCESSOR_ARCHITECTURE_IA64:
		return "Intel Itanium";
	case PROCESSOR_ARCHITECTURE_INTEL:
		return "x86";
	case PROCESSOR_ARCHITECTURE_UNKNOWN:
		return "Undefined architecture";
	default:
		return "Undefined architecture (" + std::to_string(architecture) + ")";
	}
}

void PrintCoreInfo()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	std::cout << "Architecture: " << GetProcessorArchitectureStr(systemInfo.wProcessorArchitecture) << std::endl;
	std::cout << "Processors: " << systemInfo.dwNumberOfProcessors << std::endl;
}

void PrintComputerName()
{
	DWORD bufferSize = 0;

	GetComputerNameExW(ComputerNamePhysicalDnsFullyQualified, nullptr, &bufferSize);

	if (GetLastError() != ERROR_MORE_DATA)
	{
		throw new std::runtime_error("Failed to get computer name buffer size");
	}

	std::vector<char> buffer(bufferSize);
	if (GetComputerNameExA(ComputerNamePhysicalDnsFullyQualified, buffer.data(), &bufferSize))
	{
		std::cout << "Computer name: " << std::string(buffer.data()) << std::endl;
	}
}

void PrintUserName()
{
	wchar_t username[UNLEN + 1];
	DWORD size = sizeof(username) / sizeof(wchar_t);

	if (GetUserNameW(username, &size))
	{
		std::wcout << "User: " << username << std::endl;
	}
	else
	{
		throw new std::runtime_error("Failed to get user name");
	}

}

void PrintDrivesInfo()
{
	auto bufferSize = GetLogicalDriveStrings(0, nullptr);
	std::vector<char> buffer(bufferSize);

	char* drive = buffer.data();
	while (*drive != '\0')
	{
		auto driveTipe = GetDriveType(drive);

		ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;

		if (GetDiskFreeSpaceEx(drive, &freeBytes, &totalBytes, &totalFreeBytes))
		{
			std::cout << "Drive " << drive << " (" << driveTipe << "): " << std::endl;
			std::cout << (freeBytes.QuadPart / (BYTES_IN_KB * BYTES_IN_KB * BYTES_IN_KB)) << " GB /"
					  << (totalBytes.QuadPart / (BYTES_IN_KB * BYTES_IN_KB * BYTES_IN_KB)) << " GB";
		}
	}
}
