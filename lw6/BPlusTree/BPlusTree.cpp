#include "BPlusTree.h"

#include <filesystem>
#include <utility>
#include <vector>

constexpr std::string NOT_FOUND = "NOT FOUND";
constexpr int BYTE_IN_MB = 1024 * 1024;
constexpr PID BLOCK_SIZE = 1024;

void AssertValueSize(const std::string& value);
int SearchInLeaf(uint8_t* page, KEY key);
PID SearchInternalNode(uint8_t* page, KEY key);
void RemoveKeyFromInternal(uint8_t* page, KEY key, PID oldChildPid);

BPlusTree::BPlusTree(std::string sourceFile, std::ostream& output)
	: m_filePath(std::move(sourceFile))
	, m_output(output)
{
	const bool fileExists = std::filesystem::exists(m_filePath);

	const int fd = open(
		m_filePath.c_str(),
		O_RDWR | O_CREAT,
		static_cast<mode_t>(0600));

	if (fd == -1)
	{
		throw std::runtime_error("Failed to open file");
	}
	m_fileDescriptor.Set(fd);

	if (!fileExists || GetMapFileSize() < PAGE_SIZE)
	{
		InitSuperPage();
	}
	else
	{
		MapFile();
	}
	m_isInitialized = true;
}

void BPlusTree::InitSuperPage()
{
	ExtendFileSize(1);

	std::memset(m_mapBase, 0, PAGE_SIZE);
	std::memcpy(m_superPage->magic, "BPL1", 4);

	m_superPage->version = 1;
	m_superPage->pageSize = PAGE_SIZE;
	m_superPage->rootPage = NULL_PAGE;
	m_superPage->height = 0;
	m_superPage->orderLeaf = M_LEAF;
	m_superPage->orderInt = M_INT;
	m_superPage->freeHead = NULL_PAGE;
	m_superPage->nextPid = 1;
	m_superPage->keysCount = 0;
	m_superPage->nodesCount = 0;

	FlushData(m_superPage, PAGE_SIZE);
}

void BPlusTree::Get(const KEY key) const
{
	if (!IsRootInitialized())
	{
		m_output << NOT_FOUND << std::endl;
		return;
	}

	const auto leaf = FindLeaf(key);
	if (leaf == nullptr)
	{
		m_output << NOT_FOUND << std::endl;
		return;
	}

	const auto header = reinterpret_cast<NodeHeader*>(leaf);
	const auto content = reinterpret_cast<Leaf*>(leaf + LEAF_CONTENT_SHIFT);

	auto index = SearchInLeaf(leaf, key);

	if (index < header->numKeys && content[index].key == key)
	{
		auto& [size, data] = content[index].value;
		m_output << std::string(data, size) << std::endl;
	}
	else
	{
		m_output << NOT_FOUND << std::endl;
	}
}

void BPlusTree::Put(const KEY key, const std::string& value)
{
	AssertValueSize(value);

	if (m_superPage->rootPage == NULL_PAGE) // создать дерево
	{
		InitRootPage(key, value);
		m_output << "OK" << std::endl;
		return;
	}

	const auto leafPage = FindLeaf(key);
	const auto header = reinterpret_cast<NodeHeader*>(leafPage);
	const auto content = reinterpret_cast<Leaf*>(leafPage + LEAF_CONTENT_SHIFT);

	const int index = SearchInLeaf(leafPage, key);

	if (index < header->numKeys && content[index].key == key) // обновить существующий ключ
	{
		auto& [size, data] = content[index].value;
		size = value.length();
		std::memcpy(data, value.c_str(), value.length());
		FlushData(leafPage, PAGE_SIZE);
		m_output << "OK (Updated)" << std::endl;
		return;
	}

	if (header->numKeys < M_LEAF) // вставить в существующий лист
	{
		std::memmove(
			&content[index + 1],
			&content[index],
			(header->numKeys - index) * sizeof(Leaf));

		content[index].key = key;
		content[index].value.size = value.length();
		std::memcpy(content[index].value.data, value.c_str(), value.length());

		header->numKeys++;
		m_superPage->keysCount++;

		FlushData(leafPage, PAGE_SIZE);
		FlushData(m_superPage, PAGE_SIZE);
		m_output << "OK" << std::endl;
		return;
	}

	InsertWithSplit(header, content, key, value, leafPage, index);
}

void BPlusTree::InsertWithSplit(
	NodeHeader* header,
	Leaf* content,
	const KEY key,
	const std::string& value,
	uint8_t* leafPage,
	const int index)
{
	const auto newLeafPid = AllocatePage();
	const auto newLeafPage = GetPage(newLeafPid);
	const auto newHeader = reinterpret_cast<NodeHeader*>(newLeafPage);
	const auto newContent = reinterpret_cast<Leaf*>(newLeafPage + LEAF_CONTENT_SHIFT);

	newHeader->nodeType = NodeType::LeafNode;
	newHeader->parentId = header->parentId;

	std::vector<Leaf> tempContent(M_LEAF + 1);
	std::memcpy(tempContent.data(), content, header->numKeys * sizeof(Leaf));

	Leaf newLeaf{};
	newLeaf.key = key;
	newLeaf.value.size = value.length();
	std::memcpy(newLeaf.value.data, value.c_str(), value.length());
	tempContent.insert(tempContent.begin() + index, newLeaf);

	constexpr auto splitPlace = M_LEAF_MIN;
	const auto splitKey = tempContent[splitPlace].key;

	header->numKeys = splitPlace;
	std::memcpy(content, tempContent.data(), splitPlace * sizeof(Leaf));

	newHeader->numKeys = M_LEAF + 1 - splitPlace;
	std::memcpy(newContent, tempContent.data() + splitPlace, newHeader->numKeys * sizeof(Leaf));

	newHeader->prevLeaf = GetPagePid(leafPage);
	newHeader->nextLeaf = header->nextLeaf;
	header->nextLeaf = newLeafPid;

	if (newHeader->nextLeaf != NULL_PAGE)
	{
		const auto nextHeader = reinterpret_cast<NodeHeader*>(GetPage(newHeader->nextLeaf));
		nextHeader->prevLeaf = newLeafPid;
		FlushData(nextHeader, PAGE_SIZE);
	}

	FlushData(leafPage, PAGE_SIZE);
	FlushData(newLeafPage, PAGE_SIZE);
	m_superPage->keysCount++;

	InsertIntoParent(leafPage, splitKey, newLeafPid);
	m_output << "OK (Split occurred)" << std::endl;
}

void BPlusTree::Delete(const KEY key)
{
	if (!IsRootInitialized())
	{
		m_output << NOT_FOUND << std::endl;
		return;
	}

	const auto leafPage = FindLeaf(key);
	if (leafPage == nullptr)
	{
		m_output << NOT_FOUND << std::endl;
		return;
	}

	const auto header = reinterpret_cast<NodeHeader*>(leafPage);
	const auto content = reinterpret_cast<Leaf*>(leafPage + LEAF_CONTENT_SHIFT);

	const int index = SearchInLeaf(leafPage, key);
	if (index >= header->numKeys || content[index].key != key)
	{
		m_output << NOT_FOUND << std::endl;
		return;
	}

	RemoveFromLeaf(leafPage, index);
	if (header->numKeys >= M_LEAF_MIN)
	{
		m_output << "OK (Deleted successfully)" << std::endl;
		return;
	}
	if (header->parentId == NULL_PAGE && header->numKeys > 0)
	{
		return;
	}
	if (header->parentId == NULL_PAGE && header->numKeys == 0)
	{
		FreePage(GetPagePid(leafPage));
		m_superPage->rootPage = NULL_PAGE;
		m_superPage->height = 0;
		FlushData(m_superPage, PAGE_SIZE);
		m_output << "Tree is fully cleared" << std::endl;
		return;
	}

	MergeAfterDelete(header, content, leafPage);
}

void BPlusTree::MergeAfterDelete(NodeHeader* header, Leaf* content, uint8_t* leafPage)
{
	const auto currentPid = GetPagePid(leafPage);
	const auto nextPid = header->nextLeaf;
	const auto parentPid = header->parentId;

	if (nextPid == NULL_PAGE)
	{
		return;
	}

	const auto nextPage = GetPage(nextPid);
	const auto nextHeader = reinterpret_cast<NodeHeader*>(nextPage);

	if (header->numKeys + nextHeader->numKeys > M_LEAF)
	{
		return;
	}
	const auto nextContent = reinterpret_cast<Leaf*>(nextPage + LEAF_CONTENT_SHIFT);
	std::memcpy(content + header->numKeys, nextContent, nextHeader->numKeys * sizeof(Leaf));

	header->numKeys += nextHeader->numKeys;
	header->nextLeaf = nextHeader->nextLeaf;

	if (nextHeader->nextLeaf != NULL_PAGE)
	{
		const auto nextNextHeader = reinterpret_cast<NodeHeader*>(GetPage(nextHeader->nextLeaf));
		nextNextHeader->prevLeaf = currentPid;
		FlushData(nextNextHeader, PAGE_SIZE);
	}

	const KEY keyToDeleteInParent = nextContent[0].key;

	FreePage(nextPid);
	FlushData(leafPage, PAGE_SIZE);

	RemoveKeyFromInternal(GetPage(parentPid), keyToDeleteInParent, nextPid);
	m_output << "OK (Value deleted and leafs merged)" << std::endl;
}

BPlusTree::~BPlusTree()
{
	if (m_isInitialized && m_mapBase != nullptr)
	{
		FlushData(m_mapBase, m_mapSize);
		UnmapMapFile();
	}
}

PID BPlusTree::GetPagePid(const uint8_t* pagePtr) const
{
	return (pagePtr - static_cast<uint8_t*>(m_mapBase)) / PAGE_SIZE;
}

size_t BPlusTree::GetMapFileSize() const
{
	struct stat st{};
	if (fstat(m_fileDescriptor, &st) == -1)
	{
		throw std::runtime_error("Failed to get file stats (fstat)");
	}
	return st.st_size;
}

void BPlusTree::MapFile()
{
	if (m_region != nullptr)
	{
		UnmapMapFile();
	}

	m_mapSize = GetMapFileSize();
	if (m_mapSize == 0)
	{
		throw std::runtime_error("Failed to get file map size");
	}

	if (m_fileMap == nullptr)
	{
		m_fileMap = std::make_unique<boost::interprocess::file_mapping>(
			m_filePath.c_str(),
			boost::interprocess::read_write);
	}
	try
	{
		m_region = std::make_unique<boost::interprocess::mapped_region>(
			*m_fileMap,
			boost::interprocess::read_write,
			0,
			m_mapSize);
	}
	catch (const boost::interprocess::interprocess_exception& e)
	{
		throw std::runtime_error("Failed to map region: " + std::string(e.what()));
	}

	m_mapBase = m_region->get_address();
	m_superPage = static_cast<InfoPage*>(m_mapBase);
}

void BPlusTree::UnmapMapFile()
{
	if (m_region != nullptr)
	{
		m_region.reset();
	}

	m_mapBase = nullptr;
	m_mapSize = 0;
	m_superPage = nullptr;
}

void BPlusTree::FlushData(void* address, const size_t length) const
{
	if (m_region == nullptr || address == nullptr || length == 0)
	{
		return;
	}

	const auto offset = static_cast<uint8_t*>(address) - static_cast<uint8_t*>(m_mapBase);
	try
	{
		m_region->flush(offset, length, true);
	}
	catch (const boost::interprocess::interprocess_exception& e)
	{
		m_output << "Failed to flush data: " << e.what() << std::endl;
	}
}

void BPlusTree::ExtendFileSize(const PID newPid)
{
	const auto newSize = newPid * PAGE_SIZE;

	UnmapMapFile();

	if (ftruncate(m_fileDescriptor, newSize) == -1)
	{
		throw std::runtime_error("Failed to truncate file size");
	}

	MapFile();
	m_superPage->nextPid = newPid;
	FlushData(m_superPage, PAGE_SIZE);
}

uint8_t* BPlusTree::GetPage(const PID pid) const
{
	if (pid == NULL_PAGE || pid >= m_superPage->nextPid)
	{
		return nullptr;
	}
	return static_cast<uint8_t*>(m_mapBase) + (pid * PAGE_SIZE);
}

PID BPlusTree::AllocatePage()
{
	PID newPid;

	if (m_superPage->freeHead != NULL_PAGE)
	{
		newPid = m_superPage->freeHead;
		const auto newPage = GetPage(newPid);

		const auto nextFree = reinterpret_cast<PID*>(newPage);
		m_superPage->freeHead = *nextFree;

		m_superPage->nodesCount++;
		FlushData(m_superPage, PAGE_SIZE);
		return newPid;
	}

	newPid = m_superPage->nextPid;
	const auto blockStartPid = newPid;
	const auto blockEndPid = newPid + BLOCK_SIZE;

	ExtendFileSize(blockEndPid);
	for (auto pid = blockStartPid; pid < blockEndPid; ++pid)
	{
		if (pid == blockStartPid)
		{
			m_superPage->freeHead = pid;
		}
		const auto page = GetPage(pid);
		const auto header = reinterpret_cast<NodeHeader*>(page);
		header->nodeType = NodeType::FreeNode;

		const auto nextFree = reinterpret_cast<PID*>(page);
		if (pid < blockEndPid - 1)
		{
			*nextFree = pid + 1;
		}
		else
		{
			*nextFree = NULL_PAGE;
		}
	}
	FlushData(GetPage(blockStartPid), 1024 * PAGE_SIZE);
	return AllocatePage();
}

void BPlusTree::FreePage(const PID pid) const
{
	if (pid == NULL_PAGE)
	{
		return;
	}
	const auto page = GetPage(pid);
	const auto header = reinterpret_cast<NodeHeader*>(page);
	header->nodeType = NodeType::FreeNode;

	const auto nextFree = reinterpret_cast<PID*>(page);
	*nextFree = m_superPage->freeHead;
	m_superPage->freeHead = pid;

	m_superPage->nodesCount--;
	FlushData(m_superPage, PAGE_SIZE);
}

int SearchInLeaf(uint8_t* page, const KEY key)
{
	const auto header = reinterpret_cast<NodeHeader*>(page);
	const auto content = reinterpret_cast<Leaf*>(page + LEAF_CONTENT_SHIFT);

	int low = 0;
	int high = header->numKeys - 1;
	while (low <= high)
	{
		const int mid = low + (high - low) / 2;
		if (key == content[mid].key)
		{
			return mid;
		}
		if (content[mid].key < key)
		{
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}
	return low;
}

PID SearchInternalNode(uint8_t* page, const KEY key)
{
	const auto header = reinterpret_cast<NodeHeader*>(page);
	const auto payload = reinterpret_cast<InternalNode*>(page + LEAF_CONTENT_SHIFT);

	int low = 0;
	int high = header->numKeys - 1;

	while (low <= high)
	{
		const int mid = low + (high - low) / 2;
		if (key == payload->keys[mid])
		{
			return payload->children[mid + 1];
		}
		if (payload->keys[mid] < key)
		{
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}
	return payload->children[low];
}

uint8_t* BPlusTree::FindLeaf(const KEY key) const
{
	if (m_superPage->rootPage == NULL_PAGE)
	{
		return nullptr;
	}

	auto currentPid = m_superPage->rootPage;
	auto currentPage = GetPage(currentPid);
	auto header = reinterpret_cast<NodeHeader*>(currentPage);

	while (header->nodeType != NodeType::LeafNode)
	{
		currentPid = SearchInternalNode(currentPage, key);
		currentPage = GetPage(currentPid);
		header = reinterpret_cast<NodeHeader*>(currentPage);
	}
	return currentPage;
}

void BPlusTree::InsertIntoParent(uint8_t* page, const KEY key, const PID newChildPid)
{
	const auto header = reinterpret_cast<NodeHeader*>(page);
	const auto parentPid = header->parentId;

	if (parentPid == NULL_PAGE)
	{
		CreateNewRoot(key, newChildPid);
		return;
	}

	const auto parentPage = GetPage(parentPid);
	const auto parentHeader = reinterpret_cast<NodeHeader*>(parentPage);
	const auto parentPayload = reinterpret_cast<InternalNode*>(parentPage + LEAF_CONTENT_SHIFT);

	int keyIndexToInsert = 0;
	while (keyIndexToInsert < parentHeader->numKeys && parentPayload->keys[keyIndexToInsert] < key)
	{
		keyIndexToInsert++;
	}
	const int ptrIndexToInsert = keyIndexToInsert + 1;

	if (parentHeader->numKeys < M_INT)
	{
		std::memmove(&parentPayload->keys[keyIndexToInsert + 1],
			&parentPayload->keys[keyIndexToInsert],
			(parentHeader->numKeys - keyIndexToInsert) * sizeof(KEY));

		std::memmove(&parentPayload->children[ptrIndexToInsert + 1],
			&parentPayload->children[ptrIndexToInsert],
			(parentHeader->numKeys + 1 - ptrIndexToInsert) * sizeof(PID));

		parentPayload->keys[keyIndexToInsert] = key;
		parentPayload->children[ptrIndexToInsert] = newChildPid;
		parentHeader->numKeys++;

		FlushData(parentPage, PAGE_SIZE);
	}
	else
	{
		InsertWithParentSplit(
			parentPayload,
			parentHeader,
			parentPage,
			keyIndexToInsert,
			key,
			ptrIndexToInsert,
			newChildPid);
	}
}

void BPlusTree::InsertWithParentSplit(
	InternalNode* parentPayload,
	NodeHeader* parentHeader,
	uint8_t* parentPage,
	const int keyIndexToInsert,
	const KEY key,
	const int ptrIndexToInsert,
	const PID newChildPid)
{
	std::vector<KEY> tempKeys(M_INT + 1);
	std::vector<PID> tempChildren(M_INT + 2);

	std::memcpy(tempKeys.data(), parentPayload->keys, parentHeader->numKeys * sizeof(KEY));
	std::memcpy(tempChildren.data(), parentPayload->children, (parentHeader->numKeys + 1) * sizeof(PID));

	tempKeys.insert(tempKeys.begin() + keyIndexToInsert, key);
	tempChildren.insert(tempChildren.begin() + ptrIndexToInsert, newChildPid);

	constexpr int splitPoint = M_INT_MIN;
	const KEY promotedKey = tempKeys[splitPoint];

	const PID newIntPid = AllocatePage();
	const auto newIntPage = GetPage(newIntPid);
	const auto newIntHeader = reinterpret_cast<NodeHeader*>(newIntPage);
	const auto newIntPayload = reinterpret_cast<InternalNode*>(newIntPage + LEAF_CONTENT_SHIFT);

	newIntHeader->nodeType = NodeType::InternalNode;
	newIntHeader->parentId = parentHeader->parentId;

	parentHeader->numKeys = splitPoint;
	std::memcpy(parentPayload->keys, tempKeys.data(), parentHeader->numKeys * sizeof(KEY));
	std::memcpy(parentPayload->children, tempChildren.data(), (parentHeader->numKeys + 1) * sizeof(PID));

	newIntHeader->numKeys = M_INT - splitPoint;
	std::memcpy(newIntPayload->keys,
		tempKeys.data() + splitPoint + 1,
		newIntHeader->numKeys * sizeof(KEY));

	std::memcpy(newIntPayload->children,
		tempChildren.data() + splitPoint + 1,
		(newIntHeader->numKeys + 1) * sizeof(PID));

	for (int i = 0; i <= newIntHeader->numKeys; ++i)
	{
		const PID childPid = newIntPayload->children[i];
		const auto childHeader = reinterpret_cast<NodeHeader*>(GetPage(childPid));
		childHeader->parentId = newIntPid;
		FlushData(childHeader, PAGE_SIZE);
	}

	FlushData(parentPage, PAGE_SIZE);
	FlushData(newIntPage, PAGE_SIZE);

	InsertIntoParent(parentPage, promotedKey, newIntPid);
}

void BPlusTree::CreateNewRoot(
	const KEY key,
	const PID newChildPid)
{
	const auto oldRootPid = m_superPage->rootPage;
	const auto newRootPid = AllocatePage();

	const auto newRootPage = GetPage(newRootPid);
	const auto newRootHeader = reinterpret_cast<NodeHeader*>(newRootPage);
	const auto newPayload = reinterpret_cast<InternalNode*>(newRootPage + LEAF_CONTENT_SHIFT);

	newRootHeader->nodeType = NodeType::InternalNode;
	newRootHeader->numKeys = 1;
	newRootHeader->parentId = NULL_PAGE;

	newPayload->children[0] = oldRootPid;
	newPayload->keys[0] = key;
	newPayload->children[1] = newChildPid;

	m_superPage->rootPage = newRootPid;
	m_superPage->height++;

	const auto oldRootHeader = reinterpret_cast<NodeHeader*>(GetPage(oldRootPid));
	const auto newChildHeader = reinterpret_cast<NodeHeader*>(GetPage(newChildPid));
	oldRootHeader->parentId = newRootPid;
	newChildHeader->parentId = newRootPid;

	FlushData(newRootPage, PAGE_SIZE);
	FlushData(GetPage(oldRootPid), PAGE_SIZE);
	FlushData(GetPage(newChildPid), PAGE_SIZE);
	FlushData(m_superPage, PAGE_SIZE);
}

void BPlusTree::RemoveFromLeaf(uint8_t* page, const int index) const
{
	const auto header = reinterpret_cast<NodeHeader*>(page);
	const auto content = reinterpret_cast<Leaf*>(page + LEAF_CONTENT_SHIFT);

	std::memmove(
		&content[index],
		&content[index + 1],
		(header->numKeys - 1 - index) * sizeof(Leaf));

	header->numKeys--;
	m_superPage->keysCount--;

	FlushData(page, PAGE_SIZE);
	FlushData(m_superPage, PAGE_SIZE);
}

bool BPlusTree::IsRootInitialized() const
{
	return m_superPage->rootPage != NULL_PAGE;
}

void BPlusTree::InitRootPage(const KEY key, const std::string& value)
{
	const auto newPid = AllocatePage();
	const auto newPage = GetPage(newPid);

	const auto header = reinterpret_cast<NodeHeader*>(newPage);
	const auto content = reinterpret_cast<Leaf*>(newPage + LEAF_CONTENT_SHIFT);

	header->nodeType = NodeType::LeafNode;
	header->numKeys = 1;
	header->parentId = NULL_PAGE;

	content[0].key = key;
	content[0].value.size = value.length();
	std::memcpy(content[0].value.data, value.c_str(), value.length());

	m_superPage->rootPage = newPid;
	m_superPage->height = 1;
	m_superPage->keysCount = 1;

	FlushData(newPage, PAGE_SIZE);
	FlushData(m_superPage, PAGE_SIZE);
}

void BPlusTree::Stats() const
{
	m_output << "--- B+ Tree Statistics ---" << std::endl;
	m_output << "File: " << m_filePath << std::endl;
	m_output << "Magic/Version: " << std::string(m_superPage->magic, 4) << " / " << m_superPage->version << std::endl;
	m_output << "Page Size: " << m_superPage->pageSize << " bytes" << std::endl;
	m_output << "Root Page ID: " << m_superPage->rootPage << std::endl;
	m_output << "Height: " << m_superPage->height << std::endl;
	m_output << "Max Leaf Records (M_leaf): " << m_superPage->orderLeaf << " (Min: " << M_LEAF_MIN << ")" << std::endl;
	m_output << "Max Internal Keys (M_int): " << m_superPage->orderInt << " (Min: " << M_INT_MIN << ")" << std::endl;
	m_output << "Total Keys: " << m_superPage->keysCount << std::endl;
	m_output << "Total Nodes (used pages): " << m_superPage->nodesCount << std::endl;
	m_output << "Total Pages (file size): " << m_superPage->nextPid << std::endl;
	m_output << "File Size: " << (m_superPage->nextPid * PAGE_SIZE) / BYTE_IN_MB << " MB" << std::endl;
	m_output << "Free List Head: " << m_superPage->freeHead << std::endl;

	if (m_superPage->nodesCount > 0)
	{
		const auto avgFill = static_cast<double>(m_superPage->keysCount) * LEAF_SIZE / static_cast<double>(m_superPage->nodesCount * PAGE_SIZE);
		std::cout << "Overall Fill Factor (Data/Total): "
				  << std::fixed << std::setprecision(2)
				  << avgFill * 100 << "%"
				  << std::endl;
	}
}

void AssertValueSize(const std::string& value)
{
	if (value.length() > MAX_VALUE_LEN)
	{
		throw std::runtime_error("Value too long");
	}
}

void BPlusTree::RemoveKeyFromInternal(uint8_t* page, KEY key, const PID oldChildPid)
{
	const auto header = reinterpret_cast<NodeHeader*>(page);
	const auto payload = reinterpret_cast<InternalNode*>(page + LEAF_CONTENT_SHIFT);

	int ptrIndexToRemove = -1;

	for (int i = 0; i <= header->numKeys; ++i)
	{
		if (payload->children[i] == oldChildPid)
		{
			ptrIndexToRemove = i;
			break;
		}
	}

	if (ptrIndexToRemove == -1 || ptrIndexToRemove == 0)
	{
		std::cerr << "Warning: Failed to find old child PID or attempted P0 deletion." << std::endl;
		return;
	}

	const int keyIndexToRemove = ptrIndexToRemove - 1;

	std::memmove(&payload->keys[keyIndexToRemove],
		&payload->keys[keyIndexToRemove + 1],
		(header->numKeys - 1 - keyIndexToRemove) * sizeof(KEY));

	std::memmove(&payload->children[ptrIndexToRemove],
		&payload->children[ptrIndexToRemove + 1],
		(header->numKeys - ptrIndexToRemove) * sizeof(PID));

	header->numKeys--;
	FlushData(page, PAGE_SIZE);

	if (header->numKeys >= M_INT_MIN)
	{
		return;
	}

	if (header->parentId == NULL_PAGE)
	{
		if (header->numKeys != 0)
		{
			return;
		}
		const PID newRootPid = payload->children[0];

		const auto newRootHeader = reinterpret_cast<NodeHeader*>(GetPage(newRootPid));
		newRootHeader->parentId = NULL_PAGE;
		FlushData(GetPage(newRootPid), PAGE_SIZE);

		m_superPage->rootPage = newRootPid;
		m_superPage->height--;
		FlushData(m_superPage, PAGE_SIZE);

		FreePage(GetPagePid(page));
		return;
	}

	uint8_t* parentPage = GetPage(header->parentId);
	const auto parentHeader = reinterpret_cast<NodeHeader*>(parentPage);
	const auto parentPayload = reinterpret_cast<InternalNode*>(parentPage + LEAF_CONTENT_SHIFT);

	const auto currentPid = GetPagePid(page);
	int currentPtrIndex = -1;
	for (int i = 0; i <= parentHeader->numKeys; ++i)
	{
		if (parentPayload->children[i] == currentPid)
		{
			currentPtrIndex = i;
			break;
		}
	}

	if (currentPtrIndex < parentHeader->numKeys)
	{
		const auto rightPid = parentPayload->children[currentPtrIndex + 1];
		uint8_t* rightPage = GetPage(rightPid);
		const auto rightHeader = reinterpret_cast<NodeHeader*>(rightPage);

		if (header->numKeys + 1 + rightHeader->numKeys <= M_INT)
		{
			MergeInternalNodes(page, rightPage, currentPtrIndex);
		}
	}
	else if (currentPtrIndex > 0)
	{
		const auto leftPid = parentPayload->children[currentPtrIndex - 1];
		uint8_t* leftPage = GetPage(leftPid);
		const auto leftHeader = reinterpret_cast<NodeHeader*>(leftPage);

		if (leftHeader->numKeys + 1 + header->numKeys <= M_INT)
		{
			MergeInternalNodes(leftPage, page, currentPtrIndex - 1);
		}
	}
}

void BPlusTree::MergeInternalNodes(uint8_t* leftPage, uint8_t* rightPage, int parentKeyIndex)
{
	const auto leftHeader = reinterpret_cast<NodeHeader*>(leftPage);
	const auto leftPayload = reinterpret_cast<InternalNode*>(leftPage + LEAF_CONTENT_SHIFT);

	const auto rightHeader = reinterpret_cast<NodeHeader*>(rightPage);
	const auto rightPayload = reinterpret_cast<InternalNode*>(rightPage + LEAF_CONTENT_SHIFT);

	uint8_t* parentPage = GetPage(leftHeader->parentId);
	const auto parentHeader = reinterpret_cast<NodeHeader*>(parentPage);
	const auto parentPayload = reinterpret_cast<InternalNode*>(parentPage + LEAF_CONTENT_SHIFT);

	leftPayload->keys[leftHeader->numKeys] = parentPayload->keys[parentKeyIndex];
	leftHeader->numKeys++;

	std::memcpy(&leftPayload->keys[leftHeader->numKeys],
		rightPayload->keys,
		rightHeader->numKeys * sizeof(KEY));
	leftHeader->numKeys += rightHeader->numKeys;

	std::memcpy(&leftPayload->children[leftHeader->numKeys - rightHeader->numKeys],
		rightPayload->children,
		(rightHeader->numKeys + 1) * sizeof(PID));

	for (int i = 0; i <= rightHeader->numKeys; ++i)
	{
		const auto childPid = rightPayload->children[i];
		const auto childHeader = reinterpret_cast<NodeHeader*>(GetPage(childPid));
		childHeader->parentId = GetPagePid(leftPage);
		FlushData(childHeader, PAGE_SIZE);
	}

	FreePage(GetPagePid(rightPage));
	FlushData(leftPage, PAGE_SIZE);

	const auto keyToDelete = parentPayload->keys[parentKeyIndex];

	std::memmove(&parentPayload->keys[parentKeyIndex],
		&parentPayload->keys[parentKeyIndex + 1],
		(parentHeader->numKeys - 1 - parentKeyIndex) * sizeof(KEY));

	std::memmove(&parentPayload->children[parentKeyIndex + 1],
		&parentPayload->children[parentKeyIndex + 2],
		(parentHeader->numKeys - (parentKeyIndex + 1)) * sizeof(PID));

	parentHeader->numKeys--;
	FlushData(parentPage, PAGE_SIZE);

	if (parentHeader->parentId != NULL_PAGE)
	{
		RemoveKeyFromInternal(parentPage, keyToDelete, GetPagePid(rightPage));
	}
	else if (parentHeader->numKeys == 0)
	{
		const PID newRootPid = parentPayload->children[0];

		const auto newRootHeader = reinterpret_cast<NodeHeader*>(GetPage(newRootPid));
		newRootHeader->parentId = NULL_PAGE;
		FlushData(newRootHeader, PAGE_SIZE);

		m_superPage->rootPage = newRootPid;
		m_superPage->height--;
		FlushData(m_superPage, PAGE_SIZE);

		FreePage(GetPagePid(parentPage));
	}
}
