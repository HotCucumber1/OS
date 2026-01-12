#pragma once
#include <cstdint>

#pragma pack(push, 1) // TODO подумать над порядком-байт (сетевой)
struct Segment
{
	int x1;
	int y1;
	int x2;
	int y2;
	uint32_t color;
};
#pragma pack(pop)
