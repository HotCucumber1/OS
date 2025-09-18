#include <string>
#include <vector>
#include <iomanip>
#include <iostream>
#include <windows.h>
#include <lmcons.h>
#include <psapi.h>
#include <VersionHelpers.h>

constexpr int BYTES_IN_KB = 1024;

void PrintWindowsVersion();
void PrintMemoryInfo();
void PrintCoreInfo();
void PrintComputerName();
void PrintUserName();
void PrintDrivesInfo();


int main()
{
	setlocale(LC_ALL,"Russian");

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

void PrintMemorySize(
	const unsigned long long usedMemory,
	const unsigned long long totalMemory,
	const std::string& infoTitle)
{
	std::cout << std::fixed << std::setprecision(2);

	std::cout << infoTitle << ": " << usedMemory << "mb / " << totalMemory << "mb ("
			<< static_cast<double>(usedMemory) / totalMemory * 100.0 << "%)" << std::endl;
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

		PrintMemorySize(totalPhysMemory - availablePhysMemory, totalPhysMemory, "RAM");
	}

	PERFORMANCE_INFORMATION perfInfo;
	perfInfo.cb = sizeof(perfInfo);

	// TODO обработка ошибок
	if (GetPerformanceInfo(&perfInfo, sizeof(PERFORMANCE_INFORMATION)))
	{
		// TODO разобраться
		std::cout << "Total virtual memory: "
			<< ((perfInfo.CommitLimit + perfInfo.PhysicalTotal) * perfInfo.PageSize / (BYTES_IN_KB * BYTES_IN_KB)) << " MB" << std::endl;

		std::cout << "Pagefile: "
			<< (perfInfo.CommitLimit * perfInfo.PageSize / (BYTES_IN_KB * BYTES_IN_KB)) << " MB" << std::endl;
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
		throw std::runtime_error("Failed to get computer name buffer size");
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
	DWORD size = UNLEN + 1;

	if (GetUserNameW(username, &size))
	{
		std::wcout << L"User: " << username << std::endl;
	}
	else
	{
		throw std::runtime_error("Failed to get user name");
	}
}

void PrintDrivesInfo()
{
    DWORD bufferSize = GetLogicalDriveStrings(0, nullptr);
    if (bufferSize == 0)
    {
        std::cout << "Error getting buffer: " << GetLastError() << std::endl;
        return;
    }

    std::vector<char> buffer(bufferSize);
    DWORD result = GetLogicalDriveStrings(bufferSize, buffer.data());
    if (result == 0)
    {
        std::cout << "Error getting drives: " << GetLastError() << std::endl;
        return;
    }

	std::cout << "-----------------" << std::endl;
	std::cout << "Drives" << std::endl;
	std::cout << "-----------------" << std::endl;

    const char* drive = buffer.data();
    while (*drive != '\0')
    {
        ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;

        if (GetDiskFreeSpaceEx(
        	drive,
        	&freeBytes,
        	&totalBytes,
        	&totalFreeBytes
        ))
        {
            std::cout << drive << " Free: " << (freeBytes.QuadPart / (BYTES_IN_KB * BYTES_IN_KB * BYTES_IN_KB)) << " GB / "
                      << "Total: " << (totalBytes.QuadPart / (BYTES_IN_KB * BYTES_IN_KB * BYTES_IN_KB)) << " GB"
                      << std::endl;
        }
        else
        {
            std::cout << "Drive " << drive << " Error getting space info: " << GetLastError() << std::endl;
        }
        drive += strlen(drive) + 1;
    }
}
