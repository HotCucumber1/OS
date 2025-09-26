#pragma once
#include <string>
#include <vector>

struct ProcessInfo
{
	pid_t pid;
	std::string procName;
	std::string userName;
	unsigned long privateMemory;
	unsigned long sharedMemory;
	int threads;
	std::string cmd;
	double cpuPercent;

	unsigned long GetTotalMemory() const
	{
		return privateMemory + sharedMemory;
	}
};

class ProcessLister
{
public:
	bool GetProcesses();
	void PrintProcessesInfo() const;

private:
	std::vector<ProcessInfo> m_processes;
};
