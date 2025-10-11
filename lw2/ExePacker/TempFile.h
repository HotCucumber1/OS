#pragma once
#include <stdexcept>
#include <string>
#include <unistd.h>

class TempFile
{
public:
	explicit TempFile(char* pattern)
		: m_fileDescriptor(mkstemp(pattern))
	{
		if (m_fileDescriptor != -1)
		{
			m_path = pattern;
		}
		else
		{
			throw std::runtime_error("Failed to create temporary file");
		}
	}

	TempFile(const TempFile&) = delete;
	TempFile& operator=(const TempFile&) = delete;

	~TempFile()
	{
		if (m_fileDescriptor != -1)
		{
			close(m_fileDescriptor);
			unlink(m_path.c_str());
		}
	}

	int Get() const
	{
		return m_fileDescriptor;
	}

private:
	int m_fileDescriptor = -1;
	std::string m_path;
};