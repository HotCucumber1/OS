#include "../Physical/PhysicalMemory.h"
#include <catch2/catch_all.hpp>

TEST_CASE("PhysicalMemory construction and configuration")
{
	SECTION("Default configuration correct size")
	{
		PhysicalMemoryConfig config{ 1024, 4096 };
		PhysicalMemory physMem(config);

		REQUIRE(physMem.GetSize() == 1024 * 4096);
		REQUIRE(physMem.GetSize() == 4 * 1024 * 1024);
	}

	SECTION("Custom configuration creates correct size")
	{
		PhysicalMemoryConfig config{ 512, 8192 };
		PhysicalMemory physMem(config);

		REQUIRE(physMem.GetSize() == 512 * 8192);
		REQUIRE(physMem.GetSize() == 4 * 1024 * 1024);
	}
}

TEST_CASE("PhysicalMemory Read8/Write8 operations")
{
	PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);

	SECTION("Write8 and Read8 work correctly")
	{
		physMem.Write8(0x1000, 0xCD);
		REQUIRE(physMem.Read8(0x1000) == 0xCD);
	}

	SECTION("Write8 and Read8 work correctly with multiple values")
	{
		physMem.Write8(0x500, 0x12);
		physMem.Write8(0x501, 0x34);
		physMem.Write8(0x502, 0x56);

		REQUIRE(physMem.Read8(0x500) == 0x12);
		REQUIRE(physMem.Read8(0x501) == 0x34);
		REQUIRE(physMem.Read8(0x502) == 0x56);
	}

	SECTION("Write8 overwrites previous value")
	{
		physMem.Write8(0x300, 0xAA);
		REQUIRE(physMem.Read8(0x300) == 0xAA);

		physMem.Write8(0x300, 0xBB);
		REQUIRE(physMem.Read8(0x300) == 0xBB);
	}
}

TEST_CASE("PhysicalMemory Read16/Write16 operations")
{
	constexpr PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);

	SECTION("Write16 and Read16 work correctly with aligned address")
	{
		physMem.Write16(0x1000, 0xABCD);
		REQUIRE(physMem.Read16(0x1000) == 0xABCD);
	}

	SECTION("Write16 and Read16 work at memory boundaries")
	{
		const uint32_t boundary = physMem.GetSize() - 2;
		physMem.Write16(boundary, 0xDEAD);
		REQUIRE(physMem.Read16(boundary) == 0xDEAD);
	}
}

TEST_CASE("PhysicalMemory Read32/Write32 operations")
{
	PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);

	SECTION("Write32 and Read32 work correctly with aligned address")
	{
		physMem.Write32(0x1000, 0xDEADBEEF);
		REQUIRE(physMem.Read32(0x1000) == 0xDEADBEEF);
	}

	SECTION("Write32 overwrites previous 16-bit value")
	{
		physMem.Write16(0x3000, 0xAAAA);
		physMem.Write32(0x3000, 0x12345678);

		REQUIRE(physMem.Read32(0x3000) == 0x12345678);
		REQUIRE(physMem.Read16(0x3000) == 0x5678);
	}
}

TEST_CASE("PhysicalMemory Read64/Write64 operations")
{
	PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);

	SECTION("Write64 and Read64 work correctly with aligned address")
	{
		physMem.Write64(0x1000, 0x0123456789ABCDEF);
		REQUIRE(physMem.Read64(0x1000) == 0x0123456789ABCDEF);
	}

	SECTION("Mixed operations work correctly")
	{
		physMem.Write8(0x4000, 0xAA);
		physMem.Write16(0x4004, 0xBBCC);
		physMem.Write32(0x4008, 0xDDEEFF11);
		physMem.Write64(0x4010, 0x2233445566778899);

		REQUIRE(physMem.Read8(0x4000) == 0xAA);
		REQUIRE(physMem.Read16(0x4004) == 0xBBCC);
		REQUIRE(physMem.Read32(0x4008) == 0xDDEEFF11);
		REQUIRE(physMem.Read64(0x4010) == 0x2233445566778899);
	}
}

TEST_CASE("PhysicalMemory alignment checks")
{
	PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);

	SECTION("Read16 with unaligned address throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Read16(0x1001), std::runtime_error);
	}

	SECTION("Write16 with unaligned address throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Write16(0x1001, 0x1234), std::runtime_error);
	}

	SECTION("Read32 with unaligned address throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Read32(0x1002), std::runtime_error);
		REQUIRE_THROWS_AS(physMem.Read32(0x1001), std::runtime_error);
	}

	SECTION("Write32 with unaligned address throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Write32(0x1002, 0x12345678), std::runtime_error);
		REQUIRE_THROWS_AS(physMem.Write32(0x1001, 0x12345678), std::runtime_error);
	}

	SECTION("Read64 with unaligned address throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Read64(0x1004), std::runtime_error);
		REQUIRE_THROWS_AS(physMem.Read64(0x1001), std::runtime_error);
	}

	SECTION("Write64 with unaligned address throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Write64(0x1004, 0x1234567890ABCDEF), std::runtime_error);
		REQUIRE_THROWS_AS(physMem.Write64(0x1001, 0x1234567890ABCDEF), std::runtime_error);
	}

	SECTION("Read8/Write8 work with any address alignment")
	{
		REQUIRE_NOTHROW(physMem.Read8(0x1001));
		REQUIRE_NOTHROW(physMem.Write8(0x1001, 0xAA));
		REQUIRE_NOTHROW(physMem.Read8(0x1002));
		REQUIRE_NOTHROW(physMem.Write8(0x1002, 0xBB));
		REQUIRE_NOTHROW(physMem.Read8(0x1003));
		REQUIRE_NOTHROW(physMem.Write8(0x1003, 0xCC));
	}
}

TEST_CASE("PhysicalMemory bounds checking")
{
	PhysicalMemoryConfig config{ 16, 1024 };
	PhysicalMemory physMem(config);
	const uint32_t memorySize = physMem.GetSize();

	SECTION("Read8 at boundary works")
	{
		REQUIRE_NOTHROW(physMem.Read8(memorySize - 1));
		REQUIRE_NOTHROW(physMem.Write8(memorySize - 1, 0xFF));
		REQUIRE(physMem.Read8(memorySize - 1) == 0xFF);
	}

	SECTION("Read8 beyond boundary throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Read8(memorySize), std::runtime_error);
	}

	SECTION("Write8 beyond boundary throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Write8(memorySize, 0xAA), std::runtime_error);
	}

	SECTION("Read16 at boundary works")
	{
		REQUIRE_NOTHROW(physMem.Read16(memorySize - 2));
		REQUIRE_NOTHROW(physMem.Write16(memorySize - 2, 0x1234));
		REQUIRE(physMem.Read16(memorySize - 2) == 0x1234);
	}

	SECTION("Read16 beyond boundary throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Read16(memorySize - 1), std::runtime_error);
	}

	SECTION("Write16 beyond boundary throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Write16(memorySize - 1, 0x1234), std::runtime_error);
	}

	SECTION("Read32 at boundary works")
	{
		REQUIRE_NOTHROW(physMem.Read32(memorySize - 4));
		REQUIRE_NOTHROW(physMem.Write32(memorySize - 4, 0x12345678));
		REQUIRE(physMem.Read32(memorySize - 4) == 0x12345678);
	}

	SECTION("Read32 beyond boundary throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Read32(memorySize - 3), std::runtime_error);
		REQUIRE_THROWS_AS(physMem.Read32(memorySize - 2), std::runtime_error);
		REQUIRE_THROWS_AS(physMem.Read32(memorySize - 1), std::runtime_error);
	}

	SECTION("Read64 at boundary works")
	{
		REQUIRE_NOTHROW(physMem.Read64(memorySize - 8));
		REQUIRE_NOTHROW(physMem.Write64(memorySize - 8, 0x1122334455667788));
		REQUIRE(physMem.Read64(memorySize - 8) == 0x1122334455667788);
	}

	SECTION("Read64 beyond boundary throws exception")
	{
		REQUIRE_THROWS_AS(physMem.Read64(memorySize - 7), std::runtime_error);
		REQUIRE_THROWS_AS(physMem.Read64(memorySize - 1), std::runtime_error);
	}
}

TEST_CASE("PhysicalMemory stress test")
{
	PhysicalMemoryConfig config{ 64, 4096 };
	PhysicalMemory physMem(config);

	SECTION("Multiple sequential writes and reads")
	{
		for (uint32_t i = 0; i < 10000; i += 8)
		{
			physMem.Write64(i, 0x0102030405060708 + i);
		}

		for (uint32_t i = 0; i < 10000; i += 8)
		{
			REQUIRE(physMem.Read64(i) == (0x0102030405060708 + i));
		}
	}

	SECTION("Random access pattern")
	{
		physMem.Write32(0, 0x11111111);
		physMem.Write32(4096, 0x22222222);
		physMem.Write32(8192, 0x33333333);
		physMem.Write16(100, 0x4444);
		physMem.Write8(200, 0x55);

		REQUIRE(physMem.Read32(0) == 0x11111111);
		REQUIRE(physMem.Read32(4096) == 0x22222222);
		REQUIRE(physMem.Read32(8192) == 0x33333333);
		REQUIRE(physMem.Read16(100) == 0x4444);
		REQUIRE(physMem.Read8(200) == 0x55);
	}
}