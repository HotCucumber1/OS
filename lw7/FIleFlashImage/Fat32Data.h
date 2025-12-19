#pragma once
#include <cstdint>
#include <string>

#pragma pack(push, 1)

struct FAT32BootSector
{
	uint8_t jmpBoot[3];
	char oemName[8];
	uint16_t bytesPerSector;
	uint8_t sectorsPerCluster;
	uint16_t reservedSectors;
	uint8_t numFats;
	uint16_t rootEntries;
	uint16_t totalSectors16;
	uint8_t media;
	uint16_t fatSize16;
	uint16_t sectorsPerTrack;
	uint16_t numHeads;
	uint32_t hiddenSectors;
	uint32_t totalSectors32;
	uint32_t fatSize32;
	uint16_t extFlags;
	uint16_t fsVersion;
	uint32_t rootCluster;
	uint16_t fsInfo;
	uint16_t backupBootSector;
	uint8_t reserved[12];
	uint8_t driveNumber;
	uint8_t winntFlags;
	uint8_t signature;
	uint32_t volumeId;
	char volumeLabel[11];
	char fsType[8];
	uint8_t bootCode[420];
	uint16_t bootSignature;

	bool IsValid() const
	{
		return bootSignature == 0xAA55;
	}
};

struct FAT32DirEntry
{
	uint8_t name[8];
	uint8_t ext[3];
	uint8_t attr;
	uint8_t ntReserved;
	uint8_t creationTimeTenth;
	uint16_t creationTime;
	uint16_t creationDate;
	uint16_t lastAccessDate;
	uint16_t firstClusterHi;
	uint16_t writeTime;
	uint16_t writeDate;
	uint16_t firstClusterLo;
	uint32_t fileSize;

	uint32_t GetFirstCluster() const
	{
		return firstClusterHi << 16 | firstClusterLo;
	}

	bool IsLongName() const
	{
		return attr == 0x0F;
	}

	bool IsDirectory() const
	{
		return (attr & 0x10) != 0;
	}

	bool IsDeleted() const
	{
		return name[0] == 0xE5;
	}

	bool IsEndOfDir() const
	{
		return name[0] == 0x00;
	}

	std::string GetShortName() const
	{
		std::string result;
		for (int i = 0; i < 8 && name[i] != ' '; i++)
		{
			result += name[i];
		}

		if (ext[0] != ' ')
		{
			result += '.';
			for (int i = 0; i < 3 && ext[i] != ' '; i++)
			{
				result += ext[i];
			}
		}
		return result;
	}
};

struct FAT32LongNameEntry
{
	uint8_t order;
	uint16_t name1[5];
	uint8_t attr;
	uint8_t type;
	uint8_t checksum;
	uint16_t name2[6];
	uint16_t zero;
	uint16_t name3[2];

	bool IsLastEntry() const
	{
		return (order & 0x40) != 0;
	}

	uint8_t GetSequenceNumber() const
	{
		return order & 0x3F;
	}
};

#pragma pack(pop)