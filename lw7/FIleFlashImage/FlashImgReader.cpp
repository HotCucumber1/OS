#include "FlashImgReader.h"
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>

template <typename T>
void PrintValue(const std::string& name, const T& value, const std::string& unit = "");

std::vector<std::string> Split(const std::string& sourceStr, char delimiter);

std::string ReadLongNamePart(FAT32LongNameEntry* lfn);

FlashImgReader::FlashImgReader(const std::string& imagePath, std::ostream& output)
	: m_imagePath(imagePath)
	, m_output(output)
{
	m_file.open(imagePath, std::ios::binary);

	if (!m_file)
	{
		throw std::runtime_error("Cannot open FAT32 image: " + imagePath);
	}
	ReadBootSector();
}

void FlashImgReader::PrintInfo(const std::string& path)
{
	const auto data = FindPath(path);

	if (data.isDirectory)
	{
		PrintDirectoryContents(data.cluster);
	}
	else
	{
		m_output << "FILE " << data.name << ' ' << data.size << std::endl;
		const auto fileData = ReadFileData(data.cluster, data.size);
		PrintHexDump(fileData);
	}
}

FlashImgReader::FileInfo FlashImgReader::FindPath(const std::string& path)
{
	const auto components = Split(path, '/');
	auto currentCluster = m_bootSector.rootCluster;

	for (size_t i = 0; i < components.size(); i++)
	{
		const auto& currComponent = components[i];

		auto found = FindInDirectory(currentCluster, currComponent);
		if (found.name.empty())
		{
			throw std::runtime_error("Path component not found: " + currComponent);
		}
		if (i == components.size() - 1)
		{
			return found;
		}

		if (!found.isDirectory)
		{
			throw std::runtime_error("There is filename in the middle of the path");
		}
		currentCluster = found.cluster;
	}

	return {
		"",
		m_bootSector.rootCluster,
		0,
		true,
		false,
	};
}

FlashImgReader::FileInfo FlashImgReader::FindInDirectory(const uint32_t cluster, const std::string& targetName)
{
	for (const auto& record : ReadDirectory(cluster))
	{
		if (record.name == targetName)
		{
			return record;
		}
	}
	return {};
}

std::vector<FlashImgReader::FileInfo> FlashImgReader::ReadDirectory(const uint32_t cluster)
{
	std::vector<FileInfo> records;
	auto currentCluster = cluster;
	std::string longNameBuf;

	while (currentCluster != 0 && currentCluster < 0x0FFFFFF8)
	{
		const auto sector = ClusterToSector(currentCluster);
		auto offset = sector * m_bootSector.bytesPerSector;

		m_file.seekg(offset, std::ios::beg);

		const auto clusterSize = GetClusterSize();
		std::vector<uint8_t> clusterData(clusterSize);
		m_file.read(reinterpret_cast<char*>(clusterData.data()), clusterSize);

		size_t pos = 0;
		while (pos + sizeof(FAT32DirEntry) <= clusterSize)
		{
			auto record = reinterpret_cast<FAT32DirEntry*>(clusterData.data() + pos);
			if (record->IsEndOfDir())
			{
				return records;
			}

			if (record->IsDeleted())
			{
				pos += sizeof(FAT32DirEntry);
				continue;
			}

			if (record->IsLongName())
			{
				const auto lfn = reinterpret_cast<FAT32LongNameEntry*>(record);
				const auto lfnPart = ReadLongNamePart(lfn);

				if (lfn->IsLastEntry())
				{
					longNameBuf = lfnPart;
				}
				else
				{
					longNameBuf += lfnPart;
				}
			}
			else
			{
				FileInfo info;
				if (!longNameBuf.empty())
				{
					info.name = longNameBuf;
					longNameBuf.clear();
				}
				else
				{
					info.name = record->GetShortName();
				}

				info.cluster = record->GetFirstCluster();
				info.size = record->fileSize;
				info.isDirectory = record->IsDirectory();
				info.isLongName = !longNameBuf.empty();

				if (info.name == "." || info.name == "..")
				{
					if (info.name == ".")
					{
						info.cluster = cluster;
					}
					else if (info.name == "..")
					{
						info.cluster = 0;
					}
				}
				records.push_back(info);
			}
			pos += sizeof(FAT32DirEntry);
		}
		currentCluster = GetNextCluster(currentCluster);
	}
	return records;
}

uint32_t FlashImgReader::GetClusterSize() const
{
	return m_bootSector.bytesPerSector * m_bootSector.sectorsPerCluster;
}

uint32_t FlashImgReader::GetNextCluster(const uint32_t currentCluster)
{
	if (currentCluster < 2)
	{
		return 0;
	}

	const uint64_t fatOffset = currentCluster * 4;
	const uint64_t fatSector = m_bootSector.reservedSectors + (fatOffset / m_bootSector.bytesPerSector);
	const uint32_t sectorOffset = fatOffset % m_bootSector.bytesPerSector;

	m_file.seekg(fatSector * m_bootSector.bytesPerSector + sectorOffset, std::ios::beg);

	uint32_t nextCluster;
	m_file.read(reinterpret_cast<char*>(&nextCluster), 4);

	return nextCluster & 0x0FFFFFFF;
}

std::string ReadLongNamePart(FAT32LongNameEntry* lfn)
{
	std::wstring wname;
	for (const auto ch : lfn->name1)
	{
		if (ch != 0xFFFF && ch != 0)
		{
			wname += ch;
		}
	}

	for (const auto ch : lfn->name2)
	{
		if (ch != 0xFFFF && ch != 0)
		{
			wname += ch;
		}
	}

	for (const auto ch : lfn->name3)
	{
		if (ch != 0xFFFF && ch != 0)
		{
			wname += ch;
		}
	}

	std::string result;
	for (const auto wc : wname)
	{
		if (wc < 128)
		{
			result += static_cast<char>(wc);
		}
	}
	return result;
}

void FlashImgReader::PrintBase() const
{
	std::cout << "\n========== FAT32 BOOT SECTOR INFO ==========\n\n";

	PrintValue("OEM Name", std::string(m_bootSector.oemName, 8));
	PrintValue("File System Type", std::string(m_bootSector.fsType, 8));

	PrintValue("Bytes per Sector", m_bootSector.bytesPerSector, "bytes");
	PrintValue("Sectors per Cluster", static_cast<uint32_t>(m_bootSector.sectorsPerCluster), "sectors");
	PrintValue("Cluster Size",
		m_bootSector.bytesPerSector * m_bootSector.sectorsPerCluster,
		"bytes");
	PrintValue("Reserved Sectors", m_bootSector.reservedSectors, "sectors");
	PrintValue("Number of FATs", static_cast<uint32_t>(m_bootSector.numFats), "");

	const auto fat_size = m_bootSector.fatSize16
		? m_bootSector.fatSize16
		: m_bootSector.fatSize32;
	PrintValue("FAT Size", fat_size, "sectors");
	PrintValue("FAT Total Size", fat_size * m_bootSector.bytesPerSector, "bytes");

	PrintValue("Root Directory First Cluster", m_bootSector.rootCluster, "");

	PrintValue("Total Sectors",
		m_bootSector.totalSectors16
			? m_bootSector.totalSectors16
			: m_bootSector.totalSectors32,
		"sectors");

	std::cout << "\nSignatures:\n";
	PrintValue("Boot Signature", m_bootSector.bootSignature, "hex");
	PrintValue("Extended Signature", static_cast<uint32_t>(m_bootSector.signature), "hex");
	PrintValue("Media Descriptor", static_cast<uint32_t>(m_bootSector.media), "hex");
}

void FlashImgReader::ReadBootSector()
{
	// const auto bytesRead = pread(m_descriptor, &m_bootSector, sizeof(m_bootSector), 0);
	m_file.seekg(0, std::ios::beg);
	m_file.read(reinterpret_cast<char*>(&m_bootSector), sizeof(m_bootSector));

	const auto bytesRead = m_file.gcount();

	if (bytesRead != sizeof(m_bootSector))
	{
		throw std::runtime_error("Failed to read boot sector");
	}

	if (!m_bootSector.IsValid())
	{
		throw std::runtime_error("Invalid boot sector signature");
	}

	if (strncmp(m_bootSector.fsType, "FAT32", 5) != 0 && strncmp(m_bootSector.fsType, "FAT32   ", 8) != 0)
	{
		throw std::runtime_error("File system may is not FAT32");
	}

	const auto sectorSize = m_bootSector.bytesPerSector;
	if (sectorSize < 512 || sectorSize > 4096 || (sectorSize & (sectorSize - 1)) != 0)
	{
		throw std::runtime_error("Invalid sector size");
	}

	const auto sectorsPerCluster = m_bootSector.sectorsPerCluster;
	if (sectorsPerCluster == 0 || (sectorsPerCluster & (sectorsPerCluster - 1)) != 0)
	{
		throw std::runtime_error("Invalid sectors per cluster");
	}
}

void FlashImgReader::PrintDirectoryContents(const uint32_t cluster)
{
	auto entries = ReadDirectory(cluster);
	for (const auto& entry : entries)
	{
		if (entry.name == "." || entry.name == ".." || entry.isDirectory)
		{
			m_output << "D " << entry.name << std::endl;
		}
		else
		{
			m_output << "F " << entry.name << " " << entry.size << std::endl;
		}
	}
}

std::vector<uint8_t> FlashImgReader::ReadFileData(const uint32_t startCluster, const uint32_t fileSize)
{
	std::vector<uint8_t> fileData;
	fileData.reserve(fileSize);

	auto curCluster = startCluster;
	uint32_t bytesRead = 0;
	uint32_t clusterSize = GetClusterSize();

	while (curCluster != 0 && curCluster < 0x0FFFFFF8 && bytesRead < fileSize && bytesRead < fileSize)
	{
		auto sector = ClusterToSector(curCluster);
		auto offset = sector * m_bootSector.bytesPerSector;

		m_file.seekg(offset, std::ios::beg);

		const auto bytesToRead = std::min(clusterSize, fileSize - bytesRead);
		std::vector<uint8_t> clusterData(bytesToRead);
		m_file.read(reinterpret_cast<char*>(clusterData.data()), bytesToRead);

		fileData.insert(fileData.end(), clusterData.begin(), clusterData.end());
		bytesRead += bytesToRead;

		curCluster = GetNextCluster(curCluster);

		if (curCluster >= 0x0FFFFFF8 || curCluster == 0)
		{
			break;
		}
	}
	if (fileData.size() > fileSize)
	{
		fileData.resize(fileSize);
	}

	return fileData;
}

void FlashImgReader::PrintHexDump(const std::vector<uint8_t>& data) const
{
	constexpr size_t bytesPerLine = 16;

	for (size_t offset = 0; offset < data.size(); offset += bytesPerLine)
	{
		m_output << std::hex << std::setw(8) << std::setfill('0')
				 << offset << ": ";

		for (size_t i = 0; i < bytesPerLine; i++)
		{
			if (offset + i < data.size())
			{
				m_output << std::hex << std::setw(2) << std::setfill('0')
						 << static_cast<int>(data[offset + i]) << " ";
			}
			else
			{
				m_output << "   ";
			}
			if (i == 7)
			{
				m_output << " ";
			}
		}

		m_output << " ";

		for (size_t i = 0; i < bytesPerLine; i++)
		{
			if (offset + i < data.size())
			{
				uint8_t byte = data[offset + i];
				if (std::isprint(byte) && byte != '\t' && byte != '\n' && byte != '\r')
				{
					m_output << static_cast<char>(byte);
				}
				else
				{
					m_output << ".";
				}
			}
			else
			{
				m_output << " ";
			}
		}

		m_output << std::dec << "\n";
	}

	m_output << std::string(60, '-') << "\n";
	m_output << "Total: " << data.size() << " bytes ("
			 << std::hex << data.size() << " hex)" << std::dec << "\n";
}

uint64_t FlashImgReader::ClusterToSector(const uint32_t cluster) const
{
	if (cluster < 2)
	{
		throw std::runtime_error("Invalid cluster number");
	}
	return m_bootSector.reservedSectors + (m_bootSector.numFats * m_bootSector.fatSize32) + (cluster - 2) * m_bootSector.sectorsPerCluster;
}

std::vector<std::string> Split(const std::string& sourceStr, const char delimiter)
{
	std::vector<std::string> result;
	std::istringstream iss(sourceStr);
	std::string component;

	while (std::getline(iss, component, delimiter))
	{
		if (!component.empty())
		{
			result.push_back(component);
		}
	}

	return result;
}

template <typename T>
void PrintValue(const std::string& name, const T& value, const std::string& unit)
{
	std::cout << std::left << std::setw(35) << name
			  << ": " << std::right << std::setw(10) << value;
	if (!unit.empty())
	{
		std::cout << " " << unit;
	}
	std::cout << std::endl;
}