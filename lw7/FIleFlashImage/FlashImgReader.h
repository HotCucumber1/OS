#pragma once
#include "Fat32Data.h"

#include <fstream>
#include <vector>

class FlashImgReader
{
public:
	FlashImgReader(const std::string& imagePath, std::ostream& output);

	void PrintBase() const;

	void PrintInfo(const std::string& path);

private:
	struct FileInfo
	{
		std::string name;
		uint32_t cluster;
		uint32_t size;
		bool isDirectory;
		bool isLongName;
	};

	void ReadBootSector();

	void PrintDirectoryContents(uint32_t cluster);

	std::vector<uint8_t> ReadFileData(uint32_t startCluster, uint32_t fileSize);

	void PrintHexDump(const std::vector<uint8_t>& data) const;

	FileInfo FindPath(const std::string& path);

	FileInfo FindInDirectory(uint32_t cluster, const std::string& targetName);

	std::vector<FileInfo> ReadDirectory(uint32_t cluster);

	uint32_t GetClusterSize() const;

	uint32_t GetNextCluster(uint32_t currentCluster);

	uint64_t ClusterToSector(uint32_t cluster) const;

	void ProcessCluster(
		uint32_t cluster,
		std::vector<FileInfo>& records,
		std::string& longNameBuf,
		uint32_t originalCluster);

	static bool IsValidClusterForDirectory(uint32_t cluster);

	static bool ProcessDirectoryEntry(
		const FAT32DirEntry* record,
		std::vector<FileInfo>& records,
		std::string& longNameBuf,
		uint32_t originalCluster);

	static void ProcessClusterEntries(
		const std::vector<uint8_t>& clusterData,
		std::vector<FileInfo>& records,
		std::string& longNameBuf,
		uint32_t originalCluster);

	static void ProcessLongNameEntry(const FAT32DirEntry* record, std::string& longNameBuf);

	static void ProcessShortNameEntry(
		const FAT32DirEntry* record,
		std::vector<FileInfo>& records,
		std::string& longNameBuf,
		uint32_t originalCluster);

	static FileInfo CreateFileInfoFromRecord(
		const FAT32DirEntry* record,
		const std::string& longNameBuf);

	static void AdjustSpecialDirectories(FileInfo& info, uint32_t originalCluster);

	std::vector<uint8_t> ReadClusterData(uint32_t cluster);

	std::string m_imagePath;
	std::ostream& m_output;
	FAT32BootSector m_bootSector{};
	std::ifstream m_file;
};
