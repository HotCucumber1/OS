#pragma once
#include <unistd.h>

class FileDescriptorRAII
{
public:
	explicit FileDescriptorRAII(const int fd = -1)
		: m_fd(fd)
	{
	}

	~FileDescriptorRAII()
	{
		if (m_fd != -1)
		{
			close(m_fd);
		}
	}

	FileDescriptorRAII(const FileDescriptorRAII&) = delete;

	FileDescriptorRAII& operator=(const FileDescriptorRAII&) = delete;

	FileDescriptorRAII(FileDescriptorRAII&& other) noexcept
		: m_fd(other.m_fd)
	{
		other.m_fd = -1;
	}

	FileDescriptorRAII& operator=(FileDescriptorRAII&& other) noexcept
	{
		if (this == &other)
		{
			if (m_fd != -1)
			{
				close(m_fd);
			}
			m_fd = other.m_fd;
			other.m_fd = -1;
		}
		return *this;
	}

	operator int() const
	{
		return m_fd;
	}

	void Set(const int fd)
	{
		if (m_fd != -1)
		{
			close(m_fd);
		}
		m_fd = fd;
	}

private:
	int m_fd;
};