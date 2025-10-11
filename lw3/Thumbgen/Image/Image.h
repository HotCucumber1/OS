#pragma once

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <string>

class Image
{
public:
	explicit Image(
		const std::string& inputPath,
		int& width,
		int& height,
		int& channels,
		const int type)
	{
		m_data = stbi_load(
			inputPath.c_str(),
			&width,
			&height,
			&channels,
			type);
	}

	const unsigned char* GetData() const
	{
		return m_data;
	}

	~Image()
	{
		stbi_image_free(m_data);
	}

	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;

private:
	unsigned char* m_data;
};