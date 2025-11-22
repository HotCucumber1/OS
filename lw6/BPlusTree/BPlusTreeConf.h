#pragma once
#include <cstdint>

using KEY = uint64_t;
using PID = uint64_t;

enum class NodeType : uint8_t
{
	InternalNode = 0,
	LeafNode = 1,
	FreeNode = 0xFF,
};

constexpr PID NULL_PAGE = 0;
constexpr int MAX_VALUE_LEN = 119;
constexpr int VALUE_RECORD_SIZE = MAX_VALUE_LEN + 1;
constexpr uint32_t PAGE_SIZE = 4096;

constexpr uint16_t LEAF_SIZE = 128;

constexpr uint16_t M_LEAF = 4032 / LEAF_SIZE; // 31
constexpr uint16_t M_LEAF_MIN = (M_LEAF + 1) / 2; // 16
constexpr uint16_t M_INT = (4032 - sizeof(PID)) / (sizeof(KEY) + sizeof(PID)); // 251
constexpr uint16_t M_INT_MIN = (M_INT + 1) / 2; // 126

constexpr uint8_t LEAF_CONTENT_SHIFT = 0x40;



#pragma pack(push, 1)

struct Value
{
	uint8_t size;
	char data[MAX_VALUE_LEN];
};

struct Leaf
{
	KEY key;
	Value value;
};

struct NodeHeader
{
	NodeType nodeType;
	uint8_t reserved1;
	uint16_t numKeys;
	uint32_t reserved2;
	PID parentId;
	PID nextLeaf;
	PID prevLeaf;
	uint64_t reservedPadding[2];
};

struct InfoPage
{
	char magic[4];
	uint32_t version;
	uint32_t pageSize;
	uint32_t reserved1;
	PID rootPage;
	uint32_t height;
	uint16_t orderLeaf;
	uint16_t orderInt;
	PID freeHead;
	PID nextPid;
	uint64_t keysCount;
	uint64_t nodesCount;
	char padding[PAGE_SIZE - LEAF_CONTENT_SHIFT];
};

struct InternalNode
{
	PID children[M_INT + 1];
	KEY keys[M_INT];
};

#pragma pack(pop)
