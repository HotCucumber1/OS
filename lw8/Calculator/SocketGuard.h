#pragma once
#include <unistd.h>

class SocketGuard
{
public:
	explicit SocketGuard(const int fd)
		: m_fd(fd)
	{
	}

	SocketGuard(const SocketGuard&) = delete;
	SocketGuard& operator=(const SocketGuard&) = delete;

	SocketGuard(SocketGuard&& other) noexcept
		: m_fd(other.m_fd)
	{
		other.m_fd = -1;
	}

	~SocketGuard()
	{
		if (m_fd != -1)
		{
			close(m_fd);
		}
	}

	int Get() const
	{
		return m_fd;
	}

	operator int() const
	{
		return m_fd;
	}

	bool IsValid() const
	{
		return m_fd != -1;
	}

	void Reset(const int newFd = -1)
	{
		if (m_fd != -1)
		{
			close(m_fd);
		}
		m_fd = newFd;
	}

private:
	int m_fd;
};