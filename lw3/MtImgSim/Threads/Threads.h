#pragma once

#include "../Image/Img.h"
#include <filesystem>

namespace Thread
{
std::vector<Img::ImgData> GetImagesInfo(
	Img::Image& queryImg,
	const std::vector<std::filesystem::path>& images,
	int threadsCount);
} // namespace Thread