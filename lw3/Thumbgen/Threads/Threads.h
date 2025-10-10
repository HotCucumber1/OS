#pragma once

#include <filesystem>
#include <vector>

void ScaleImages(
	const std::vector<std::filesystem::path>& images,
	const std::string& outputDir,
	int sdtWidth,
	int dstHeight,
	int threadsCount);