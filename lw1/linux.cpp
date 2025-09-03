#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <mntent.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>


const int BITES_IN_KB = 1024;

std::string GetPrettyName();
long GetVmallocTotal();
void PrintDriversInfo();

void PrintRamInfo(struct sysinfo &info);

int main()
{
    struct utsname buf;
    struct sysinfo info;

    char hostname[256];
    std::string prettyName = GetPrettyName();
    std::cout << "OS: " << prettyName << std::endl;

    if (uname(&buf) == 0)
    {
        std::cout << "Ядро: " << buf.release << std::endl;
    }
    PrintRamInfo(info); // TODO все по нулям почему-то

    std::cout << "Количество процессоров: " << get_nprocs() << std::endl;
    std::cout << "Архитектура процессора: " << buf.machine << std::endl;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Загрузка процессора: "
        << info.loads[0] / 65536.0 << ' '
        << info.loads[1] / 65536.0 << ' '
        << info.loads[2] / 65536.0 << ' '
        << std::endl;

    std::cout << "Пользователь: " << getlogin() << std::endl;

    if (gethostname(hostname, sizeof(std::string) - 1) == 0)
    {
        std::cout << "Хост: " << hostname << std::endl;
    }

    auto vmallocTotal = GetVmallocTotal();
    if (vmallocTotal != -1)
    {
        std::cout << "Виртуальная память: " << vmallocTotal << " kb" << std::endl;
    }

    PrintDriversInfo();
}


void PrintRamInfo(struct sysinfo &info)
{
    const double BITES_IN_MB = 1024 * 1024;

    auto totalRam = info.totalram * info.mem_unit / BITES_IN_MB;
    auto freeRam = info.freeram * info.mem_unit / BITES_IN_MB;

    if (sysinfo(&info) == 0)
    {
        std::cout << "Свободной памяти: " << freeRam << std::endl;
        std::cout << "Памяти всего: " << totalRam << std::endl;
    }
}


long GetVmallocTotal()
{
    const char separator = ':';
    const char space = ' ';

    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line))
    {
        if (line.find("VmallocTotal") == std::string::npos)
        {
            continue;
        }
        size_t pos = line.find(separator);
        if (pos == std::string::npos)
        {
            continue;
        }
        std::string valueStr = line.substr(pos + 1);

        valueStr = valueStr.substr(0, valueStr.find("kB"));

        valueStr.erase(0, valueStr.find_first_not_of(space));
        valueStr.erase(valueStr.find_last_not_of(space) + 1);

        return std::stol(valueStr) * BITES_IN_KB;
    }

    return -1;
}

std::string GetPrettyName()
{
    const char quote = '"';
    const std::string osReleaseFileName = "/etc/os-release";

    std::ifstream osRelease(osReleaseFileName);

    if (!osRelease.is_open())
    {
        throw std::runtime_error("Failed to open /etc/os-release");
    }

    std::string line;
    while (std::getline(osRelease, line))
    {
        if (line.find("PRETTY_NAME=") != 0)
        {
            continue;
        }
        size_t startQuote = line.find(quote);
        size_t endQuote = line.rfind(quote);

        return line.substr(
            startQuote + 1,
            endQuote - startQuote - 1
        );
    }

    throw std::runtime_error("PRETTY_NAME not found in /etc/os-release");
}

void PrintDriversInfo()
{
    const char* driversFile = "/proc/mounts";

    FILE* file = setmntent(driversFile, "r");

    if (file == nullptr)
    {
        return;
    }

    std::cout << "========================" << std::endl;
    std::cout << "Drivers: " << std::endl;

    struct mntent* entry;
    while ((entry = getmntent(file)) != nullptr)
    {
        struct statvfs vfs;

        if (statvfs(entry->mnt_dir, &vfs) != 0)
        {
            std::cerr << "Ошибка statvfs" << std::endl;
            return;
        }

        unsigned long long totalBytes = static_cast<unsigned long long>(vfs.f_blocks) * static_cast<unsigned long long>(vfs.f_frsize);
        unsigned long long freeBytes = static_cast<unsigned long long>(vfs.f_bfree) * static_cast<unsigned long long>(vfs.f_frsize);

        double totalGb = totalBytes / (BITES_IN_KB * BITES_IN_KB * BITES_IN_KB);
        double freeGb = freeBytes / (BITES_IN_KB * BITES_IN_KB * BITES_IN_KB);

        std::cout << entry->mnt_dir << '\t'
                  << freeGb << "gB free / "
                  << totalGb << "gb total"
                  << std::endl;
    }

    endmntent(file);
}