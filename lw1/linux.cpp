#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <climits>
#include <mntent.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>

const int BYTES_IN_KB = 1024;

void PrintKernelInfo();
void PrintOSName();
void PrintVirtualMemoryInfo();
void PrintDriversInfo();
void PrintUserInfo();

void PrintRamInfo(struct sysinfo &info);

int main()
{
    try
    {
        const double loadScale = 65536;
        struct sysinfo info;

        if (sysinfo(&info) != 0)
        {
            std::cerr << "Can not read sysinfo" << std::endl;
        }

        std::cout << std::fixed << std::setprecision(2);
        PrintDriversInfo();
        PrintOSName();
        PrintKernelInfo();
        PrintRamInfo(info);

        std::cout << "Количество процессоров: " << get_nprocs() << std::endl;

        std::cout << "Загрузка процессора: "
                  << info.loads[0] / loadScale << ' '
                  << info.loads[1] / loadScale << ' '
                  << info.loads[2] / loadScale << ' '
                  << std::endl;

        PrintVirtualMemoryInfo();
        PrintUserInfo();
    }
    catch (const std::exception& exception)
    {
        std::cout << exception.what() << std::endl;
        return 1;
    }
}

void PrintUserInfo()
{
    char hostname[HOST_NAME_MAX];

    std::cout << "Пользователь: " << getlogin() << std::endl;
    if (gethostname(hostname, HOST_NAME_MAX) == 0)
    {
        std::cout << "Хост: " << hostname << std::endl;
    }
}

// TODO обработку ошибок добавить
void PrintKernelInfo()
{
    struct utsname buf;

    if (uname(&buf) == 0)
    {
        std::cout << "Ядро: " << buf.release << std::endl;
        std::cout << "Архитектура процессора: " << buf.machine << std::endl;
    }
}


void PrintRamInfo(struct sysinfo &info)
{
    const double BITES_IN_MB = 1024 * 1024;

    auto totalRam = info.totalram * info.mem_unit / BITES_IN_MB;
    auto freeRam = info.freeram * info.mem_unit / BITES_IN_MB;

    if (sysinfo(&info) == 0)
    {
        std::cout << "Свободной памяти: " << freeRam << " mB" << std::endl;
        std::cout << "Памяти всего: " << totalRam << " mB" << std::endl;
    }
}


void PrintVirtualMemoryInfo()
{
    long totalMem = -1;
    long totalSwap = -1;

    // TODO: узнать, что будет, если читать файл, когда его меняют
    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line))
    {
        if (line.find("MemTotal:") == 0)
        {
            totalMem = std::stol(line.substr(line.find_first_of("0123456789")));
        }
        else if (line.find("SwapTotal:") == 0)
        {
            totalSwap = std::stol(line.substr(line.find_first_of("0123456789")));
        }

        if (totalMem != -1 && totalSwap != -1)
        {
            break;
        }
    }

    if (totalMem == -1 || totalSwap == -1)
    {
        std::cerr << "Can not find `MemTotal` and `SwapTotal` in /proc/meminfo" << std::endl;
    }

    long totalVirtualMemoryMb = (totalSwap) / BYTES_IN_KB;
    std::cout << "Виртуальная память: " << totalVirtualMemoryMb << " mB" << std::endl;
}

void PrintOSName()
{
    const char quote = '"';
    const std::string osReleaseFileName = "/etc/os-release";

    std::ifstream osRelease(osReleaseFileName);

    if (!osRelease.is_open())
    {
        throw std::runtime_error("Failed to open /etc/os-release");
    }

    std::string osName;
    std::string line;
    while (std::getline(osRelease, line))
    {
        if (line.find("PRETTY_NAME=") != 0)
        {
            continue;
        }
        size_t startQuote = line.find(quote);
        size_t endQuote = line.rfind(quote);

        osName = line.substr(
            startQuote + 1,
            endQuote - startQuote - 1
        );
        break;
    }

    std::cout << "OS: " << osName << std::endl;
}

void PrintDriversInfo()
{
    FILE* file = setmntent("/proc/mounts", "r");

    if (file == nullptr)
    {
        return;
    }

    std::cout << "Drivers: " << std::endl;

    struct mntent* entry;
    while ((entry = getmntent(file)) != nullptr)
    {
        struct statvfs vfs;

        if (statvfs(entry->mnt_dir, &vfs) != 0)
        {
            std::cout << "Cannot get stats for drive" << entry->mnt_dir << std::endl;
            continue;
        }

        unsigned long totalBytes = static_cast<unsigned long>(vfs.f_blocks) * static_cast<unsigned long>(vfs.f_frsize);
        unsigned long freeBytes = static_cast<unsigned long>(vfs.f_bfree) * static_cast<unsigned long>(vfs.f_frsize);

        double totalGb = totalBytes / (BYTES_IN_KB * BYTES_IN_KB * BYTES_IN_KB);
        double freeGb = freeBytes / (BYTES_IN_KB * BYTES_IN_KB * BYTES_IN_KB);

        std::cout << entry->mnt_dir << '\t'
                  << freeGb << "gB free / "
                  << totalGb << "gb total"
                  << std::endl;
    }

    endmntent(file);
    std::cout << "========================" << std::endl;
}