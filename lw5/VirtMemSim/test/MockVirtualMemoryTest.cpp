#include "../Virtual/VirtualMemory.h"
#include "MockOSHandler.h"
#include <catch2/catch_all.hpp>
#include <iostream>

TEST_CASE("VirtualMemory basic translation", "[VirtualMemory]")
{
	MockOSHandler mockHandler;
	PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);
	VirtualMemory virtMem(physMem, mockHandler);

	SECTION("Without page table uses direct mapping")
	{
		physMem.Write32(0x1234, 0xDEADBEEF);
		REQUIRE(virtMem.Read32(0x1234, Privilege::Supervisor) == 0xDEADBEEF);
	}

	SECTION("Get/Set page table address")
	{
		virtMem.SetPageTableAddress(0x5000);
		REQUIRE(virtMem.GetPageTableAddress() == 0x5000);
	}

	SECTION("Misaligned page table address causes page fault")
	{
		MockOSHandler mockHandler1;
		VirtualMemory vm(physMem, mockHandler1);

		vm.SetPageTableAddress(0x1001);

		REQUIRE(mockHandler1.pageFaultCalls.size() == 1);
		REQUIRE(mockHandler1.pageFaultCalls[0].reason == PageFaultReason::MisalignedAccess);
	}
}

TEST_CASE("VirtualMemory with page table", "[VirtualMemory]")
{
	MockOSHandler mockHandler;
	PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);
	VirtualMemory virtMem(physMem, mockHandler);

	constexpr uint32_t PAGE_TABLE_ADDR = 0x1000;
	virtMem.SetPageTableAddress(PAGE_TABLE_ADDR);

	SECTION("Page fault on non-present page")
	{
		MockOSHandler mockHandler1;
		VirtualMemory vm(physMem, mockHandler1);
		vm.SetPageTableAddress(PAGE_TABLE_ADDR);

		REQUIRE_THROWS(vm.Read32(0x2000, Privilege::Supervisor));

		REQUIRE(mockHandler1.pageFaultCalls.size() == 2);
		REQUIRE(mockHandler1.pageFaultCalls[0].reason == PageFaultReason::NotPresent);
		REQUIRE(mockHandler1.pageFaultCalls[0].access == Access::Read);
	}

	SECTION("Successful translation with present page")
	{
		PTE pte;
		pte.SetFrame(1);
		pte.SetPresent(true);
		pte.SetWritable(true);
		pte.SetUser(true);

		physMem.Write32(PAGE_TABLE_ADDR, pte.raw);
		physMem.Write32(1 * 4096 + 0x100, 0x12345678);

		REQUIRE(virtMem.Read32(0x100, Privilege::User) == 0x12345678);
	}

	SECTION("Write access sets dirty bit")
	{
		PTE pte;
		pte.SetFrame(1);
		pte.SetPresent(true);
		pte.SetWritable(true);
		pte.SetUser(true);
		pte.SetAccessed(false);
		pte.SetDirty(false);

		physMem.Write32(PAGE_TABLE_ADDR, pte.raw);
		virtMem.Write32(0x200, 0xABCD, Privilege::User);

		PTE updatedPte;
		updatedPte.raw = physMem.Read32(PAGE_TABLE_ADDR);
		REQUIRE(updatedPte.IsAccessed() == true);
		REQUIRE(updatedPte.IsDirty() == true);
	}

	SECTION("Read access sets accessed bit only")
	{
		PTE pte;
		pte.SetFrame(1);
		pte.SetPresent(true);
		pte.SetWritable(true);
		pte.SetUser(true);
		pte.SetAccessed(false);
		pte.SetDirty(false);

		physMem.Write32(PAGE_TABLE_ADDR, pte.raw);
		virtMem.Read32(0x200, Privilege::User);

		PTE updatedPte;
		updatedPte.raw = physMem.Read32(PAGE_TABLE_ADDR);
		REQUIRE(updatedPte.IsAccessed() == true);
		REQUIRE(updatedPte.IsDirty() == false);
	}
}

TEST_CASE("VirtualMemory access rights", "[VirtualMemory]")
{
	PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);
	MockOSHandler mockHandler;
	VirtualMemory virtMem(physMem, mockHandler);

	constexpr uint32_t PAGE_TABLE_ADDR = 0x1000;
	virtMem.SetPageTableAddress(PAGE_TABLE_ADDR);

	SECTION("User access to supervisor page causes fault")
	{
		PTE pte;
		pte.SetFrame(1);
		pte.SetPresent(true);
		pte.SetWritable(true);
		pte.SetUser(false);
		physMem.Write32(PAGE_TABLE_ADDR, pte.raw);

		REQUIRE_THROWS(virtMem.Read32(0x100, Privilege::User));

		REQUIRE(mockHandler.pageFaultCalls.size() == 1);
		REQUIRE(mockHandler.pageFaultCalls[0].reason == PageFaultReason::UserAccessToSupervisor);
	}

	SECTION("Supervisor access to user page works")
	{
		PTE pte;
		pte.SetFrame(1);
		pte.SetPresent(true);
		pte.SetWritable(true);
		pte.SetUser(true);
		physMem.Write32(PAGE_TABLE_ADDR, pte.raw);

		REQUIRE_NOTHROW(virtMem.Read32(0x100, Privilege::Supervisor));
	}

	SECTION("Write to read-only page causes fault")
	{
		PTE pte;
		pte.SetFrame(1);
		pte.SetPresent(true);
		pte.SetWritable(false);
		pte.SetUser(true);
		physMem.Write32(PAGE_TABLE_ADDR, pte.raw);

		REQUIRE_THROWS(virtMem.Write32(0x100, 0x1234, Privilege::User));

		REQUIRE(mockHandler.pageFaultCalls.size() == 2);
		REQUIRE(mockHandler.pageFaultCalls[0].reason == PageFaultReason::WriteToReadOnly);
	}
}

TEST_CASE("VirtualMemory misaligned access", "[VirtualMemory]")
{
	PhysicalMemoryConfig config{ 1024, 4096 };
	PhysicalMemory physMem(config);
	MockOSHandler mockHandler;
	VirtualMemory virtMem(physMem, mockHandler);

	virtMem.SetPageTableAddress(0x1000);

	SECTION("Misaligned Read16 causes page fault")
	{
		REQUIRE_THROWS(virtMem.Read16(0x1001, Privilege::User));
		REQUIRE(mockHandler.pageFaultCalls[0].reason == PageFaultReason::MisalignedAccess);
	}

	SECTION("Misaligned Read32 causes page fault")
	{
		REQUIRE_THROWS(virtMem.Read32(0x1002, Privilege::User));
		REQUIRE(mockHandler.pageFaultCalls[0].reason == PageFaultReason::MisalignedAccess);
	}

	SECTION("Misaligned Read64 causes page fault")
	{
		REQUIRE_THROWS(virtMem.Read64(0x1004, Privilege::User));
		REQUIRE(mockHandler.pageFaultCalls[0].reason == PageFaultReason::MisalignedAccess);
	}

	SECTION("Misaligned Write16 causes page fault")
	{
		REQUIRE_THROWS(virtMem.Write16(0x1001, 0x1234, Privilege::User));
		REQUIRE(mockHandler.pageFaultCalls[0].reason == PageFaultReason::MisalignedAccess);
	}

	SECTION("Misaligned Write32 causes page fault")
	{
		REQUIRE_THROWS(virtMem.Write32(0x1002, 0x1234, Privilege::User));
		REQUIRE(mockHandler.pageFaultCalls[0].reason == PageFaultReason::MisalignedAccess);
	}

	SECTION("Misaligned Write64 causes page fault")
	{
		REQUIRE_THROWS(virtMem.Write64(0x1004, 0x1234, Privilege::User));
		REQUIRE(mockHandler.pageFaultCalls[0].reason == PageFaultReason::MisalignedAccess);
	}
}