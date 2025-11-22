#pragma once
#include "BPlusTreeConf.h"
#include "FileRAII.h"

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <memory>
#include <string>

class BPlusTree
{
public:
	explicit BPlusTree(std::string sourceFile, std::ostream& output);

	void Get(KEY key) const;

	void Put(KEY key, const std::string& value);

	void Delete(KEY key);

	void Stats() const;

	~BPlusTree();

private:
	void InitSuperPage();

	PID GetPagePid(const uint8_t* pagePtr) const;

	size_t GetMapFileSize() const;

	void MapFile();

	void UnmapMapFile();

	void FlushData(void* address, size_t length) const;

	void ExtendFileSize(PID newPid);

	uint8_t* GetPage(PID pid) const;

	PID AllocatePage();

	void FreePage(PID pid) const;

	uint8_t* FindLeaf(KEY key) const;

	void InsertIntoParent(uint8_t* page, KEY key, PID newChildPid);

	void RemoveFromLeaf(uint8_t* page, int index) const;

	bool IsRootInitialized() const;

	void InitRootPage(KEY key, const std::string& value);

	void CreateNewRoot(KEY key, PID newChildPid);

	void RemoveKeyFromInternal(uint8_t* page, KEY key, PID oldChildPid);

	void MergeInternalNodes(uint8_t* leftPage, uint8_t* rightPage, int parentKeyIndex);

	void InsertWithSplit(NodeHeader* header, Leaf* content, KEY key, const std::string& value, uint8_t* leafPage, int index);

	void MergeAfterDelete(NodeHeader* header, Leaf* content, uint8_t* leafPage);

	void InsertWithParentSplit(InternalNode* parentPayload,
		NodeHeader* parentHeader,
		uint8_t* parentPage,
		int keyIndexToInsert,
		KEY key,
		int ptrIndexToInsert,
		PID newChildPid);

private:
	std::string m_filePath;
	InfoPage* m_superPage = nullptr;
	void* m_mapBase = nullptr;
	size_t m_mapSize = 0;
	bool m_isInitialized = false;
	FileDescriptorRAII m_fileDescriptor;

	std::ostream& m_output;

	std::unique_ptr<boost::interprocess::mapped_region> m_region = nullptr;
	std::unique_ptr<boost::interprocess::file_mapping> m_fileMap = nullptr;
};
