#include "ProcessLister.h"
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <pwd.h>
#include <unistd.h>

const double KB_IN_MB = 1024;
const int PATH_SIZE = 256;

struct DirDeleter
{
	void operator()(DIR* dir) const
	{
		if (dir)
		{
			closedir(dir);
		}
	}
};

using UniqueDir = std::unique_ptr<DIR, DirDeleter>;

UniqueDir open_dir(const std::string& path)
{
	DIR* dir = opendir(path.c_str());
	if (!dir)
	{
		throw std::runtime_error("Cannot open directory: " + path);
	}
	return UniqueDir(dir);
}

bool IsPid(const char* name)
{
	for (int i = 0; name[i] != '\0'; i++)
	{
		if (!isdigit(name[i]))
		{
			return false;
		}
	}
	return true;
}

bool GetProcessName(pid_t pid, std::string& name)
{
	const int exePathSize = 1024;

	char path[PATH_SIZE];
	char exePath[exePathSize];

	snprintf(path, sizeof(path), "/proc/%d/exe", pid);
	ssize_t len = readlink(path, exePath, sizeof(exePath) - 1);

	if (len == -1)
	{
		snprintf(path, sizeof(path), "/proc/%d/comm", pid);
		std::ifstream commFile(path);
		if (commFile)
		{
			std::getline(commFile, name);
			return true;
		}
		return false;
	}
	exePath[len] = '\0';
	char* basename = strrchr(exePath, '/');

	name = basename
		? basename + 1
		: exePath;

	return true;
}

bool GetProcessUser(pid_t pid, std::string& username)
{
	char statusPath[PATH_SIZE];
	snprintf(statusPath, sizeof(statusPath), "/proc/%d/status", pid);

	std::ifstream statusFile(statusPath);
	if (!statusFile)
	{
		return false;
	}

	std::string line;
	while (std::getline(statusFile, line))
	{
		if (line.find("Uid:") != 0)
		{
			continue;
		}
		std::istringstream iss(line);
		std::string label;
		uid_t uid;
		iss >> label >> uid;

		struct passwd* pw = getpwuid(uid);

		username = (pw)
			? pw->pw_name
			: std::to_string(uid);
		return true;
	}

	return false;
}

bool GetProcessMemory(pid_t pid, unsigned long& privateMem, unsigned long& sharedMem)
{
	const int privateMemSizePos = 14;
	const int sharedMemSizePos = 13;

	char smapsPath[PATH_SIZE];
	snprintf(smapsPath, sizeof(smapsPath), "/proc/%d/smaps_rollup", pid);

	std::ifstream smapsFile(smapsPath);

	unsigned long privateClean = 0;
	unsigned long privateDirty = 0;
	unsigned long sharedClean = 0;
	unsigned long sharedDirty = 0;

	std::string line;
	while (std::getline(smapsFile, line))
	{
		if (line.find("Private_Clean:") == 0)
		{
			privateClean += std::stoul(line.substr(privateMemSizePos));
		}
		else if (line.find("Private_Dirty:") == 0)
		{
			privateDirty += std::stoul(line.substr(privateMemSizePos));
		}
		else if (line.find("Shared_Clean:") == 0)
		{
			sharedClean += std::stoul(line.substr(sharedMemSizePos));
		}
		else if (line.find("Shared_Dirty:") == 0)
		{
			sharedDirty += std::stoul(line.substr(sharedMemSizePos));
		}
	}
	privateMem = privateClean + privateDirty;
	sharedMem = sharedClean + sharedDirty;

	return true;
}

void GetProcessThreads(pid_t pid, int& threads)
{
	char status_path[PATH_SIZE];
	snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);

	std::ifstream status_file(status_path);
	if (status_file)
	{
		std::string line;
		while (std::getline(status_file, line))
		{
			if (line.find("Threads:") != 0)
			{
				continue;
			}
			threads = std::stoi(line.substr(8));
			return;
		}
	}

	threads = 1;
}

void GetProcessCmd(pid_t pid, std::string& cmdline)
{
	char cmdline_path[PATH_SIZE];
	snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/cmdline", pid);

	std::ifstream cmdline_file(cmdline_path);
	if (cmdline_file)
	{
		std::getline(cmdline_file, cmdline);
		std::replace(cmdline.begin(), cmdline.end(), '\0', ' ');
		while (!cmdline.empty() && cmdline.back() == ' ')
		{
			cmdline.pop_back();
		}
		return;
	}

	cmdline = "";
}

bool GetProcessInfo(pid_t pid, ProcessInfo& processInfo)
{
	processInfo.pid = pid;

	bool hasProcName = GetProcessName(pid, processInfo.procName);
	bool hasUserName = GetProcessUser(pid, processInfo.userName);
	bool hasProcMem = GetProcessMemory(pid, processInfo.privateMemory, processInfo.sharedMemory);
	GetProcessThreads(pid, processInfo.threads);
	GetProcessCmd(pid, processInfo.cmd);

	return hasProcName && hasUserName && hasProcMem;
}

bool ProcessLister::GetProcesses()
{
	const char* procDirStr = "/proc";
	m_processes.clear();

	auto procDir = open_dir(procDirStr);

	struct dirent* entry;
	while ((entry = readdir(procDir.get())) != nullptr)
	{
		if (entry->d_type != DT_DIR || !IsPid(entry->d_name))
		{
			continue;
		}
		pid_t pid = std::stoi(entry->d_name);
		ProcessInfo info;

		if (!GetProcessInfo(pid, info))
		{
			continue;
		}
		m_processes.push_back(info);
	}

	std::sort(
		m_processes.begin(),
		m_processes.end(),
		[](const ProcessInfo& a, const ProcessInfo& b) {
			return a.GetTotalMemory() > b.GetTotalMemory();
		});

	return true;
}

void ProcessLister::PrintProcessesInfo() const
{
	const int tableSize = 200;

	const int pidShift = 8;
	const int procNameShift = 20;
	const int userNameShift = 40;
	const int threadsShift = 8;
	const int privateMemShift = 15;
	const int sharedMemShift = 15;
	const int totalMemShift = 15;
	const int cmdShift = 40;

	const int maxProcNameLen = 19;
	const int maxUserNameLen = 35;

	std::cout << std::left
			  << std::setw(pidShift) << "PID"
			  << std::setw(procNameShift) << "Name"
			  << std::setw(userNameShift) << "Username"
			  << std::setw(threadsShift) << "Threads"
			  << std::setw(privateMemShift) << "Private (MB)"
			  << std::setw(sharedMemShift) << "Shared (MB)"
			  << std::setw(totalMemShift) << "Total (MB)"
			  << std::setw(cmdShift) << "CMD"
			  << std::endl;

	std::cout << std::string(tableSize, '-') << std::endl;

	unsigned long totalPrivate = 0;
	unsigned long totalShared = 0;

	for (const auto& proc : m_processes)
	{
		auto procName = (proc.procName.length() > maxProcNameLen)
			? proc.procName.substr(0, maxProcNameLen - 2) + ".."
			: proc.procName;
		auto userName = (proc.userName.length() > maxUserNameLen)
			? proc.userName.substr(0, maxUserNameLen - 2) + ".."
			: proc.procName;

		std::cout << std::left
				  << std::setw(pidShift) << proc.pid
				  << std::setw(procNameShift) << procName
				  << std::setw(userNameShift) << userName
				  << std::setw(threadsShift) << proc.threads
				  << std::left
				  << std::setw(privateMemShift) << proc.privateMemory / KB_IN_MB
				  << std::setw(sharedMemShift) << proc.sharedMemory / KB_IN_MB
				  << std::setw(totalMemShift) << proc.GetTotalMemory() / KB_IN_MB
				  << std::setw(cmdShift) << proc.cmd
				  << std::endl;

		totalPrivate += proc.privateMemory;
		totalShared += proc.sharedMemory;
	}

	std::cout << std::string(tableSize, '=') << std::endl;
	std::cout << "Total stats:" << std::endl;
	std::cout << "Total processes: " << m_processes.size() << std::endl;
	std::cout << "Total private mem: " << totalPrivate / KB_IN_MB << " MB" << std::endl;
	std::cout << "Total shared mem: " << totalShared / KB_IN_MB << " MB" << std::endl;
	std::cout << "Total mem: " << (totalPrivate + totalShared) / KB_IN_MB << " MB" << std::endl;
}
