#include "../OSHandler/MyOS.h"
#include "../PTE/PageTableEntry.h"
#include "../Virtual/VirtualMemory.h"
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <iostream>

void SetupInitialMapping(PhysicalMemory& physMem, VirtualMemory& virtMem, uint32_t pdFrame, uint32_t ptFrame)
{
	virtMem.SetPageTableAddress(pdFrame * PTE::PAGE_SIZE);

	PTE pde;
	pde.SetPresent(true);
	pde.SetWritable(true);
	pde.SetUser(true);
	pde.SetFrame(ptFrame);

	physMem.Write32(pdFrame * PTE::PAGE_SIZE, pde.raw);

	for (uint32_t i = 0; i < PTE::PAGE_SIZE / sizeof(PTE); ++i)
	{
		physMem.Write32(ptFrame * PTE::PAGE_SIZE + i * sizeof(PTE), 0);
	}
}

TEST_CASE("MyOS Constructor and Initialization", "[MyOS][Init]")
{
	PhysicalMemoryConfig config{ 16, 4096 };
	PhysicalMemory physMem(config);
	std::filesystem::path swapPath = std::filesystem::temp_directory_path() / "os_init_test.bin";

	MyOS os(swapPath, physMem);

	SECTION("Initial counters are zero")
	{
		REQUIRE(os.GetPageFaultCount() == 0);
		REQUIRE(os.GetSwapReadCount() == 0);
		REQUIRE(os.GetSwapWriteCount() == 0);
	}
}

TEST_CASE("MyOS Page Fault - Basic NotPresent Handling", "[MyOS][PageFault]")
{
	PhysicalMemoryConfig config{ 64, 4096 };
	PhysicalMemory physMem(config);
	MyOS os(std::filesystem::temp_directory_path() / "os_fault_basic.bin", physMem);
	VirtualMemory virtMem(physMem, os);

	SetupInitialMapping(physMem, virtMem, 1, 2);

	uint32_t vpn = 1;
	uint32_t pteAddr = 2 * PTE::PAGE_SIZE + vpn * sizeof(PTE);
	physMem.Write32(pteAddr, 0);

	SECTION("Successful handling of NotPresent fault")
	{
		uint32_t virtualAddress = vpn * PTE::PAGE_SIZE + 0x100;

		REQUIRE_NOTHROW(virtMem.Read8(virtualAddress, Privilege::User, false));

		REQUIRE(os.GetPageFaultCount() == 1);

		PTE newPte;
		newPte.raw = physMem.Read32(pteAddr);
		REQUIRE(newPte.IsPresent());
		REQUIRE(newPte.GetFrame() >= 3);
	}

	SECTION("Write access sets PTE Writable")
	{
		uint32_t vpn_write = 2;
		uint32_t pteAddr_write = 2 * PTE::PAGE_SIZE + vpn_write * sizeof(PTE);
		physMem.Write32(pteAddr_write, 0);
		uint32_t virtualAddress = vpn_write * PTE::PAGE_SIZE;

		REQUIRE_NOTHROW(virtMem.Write8(virtualAddress, 0xAA, Privilege::User));

		PTE newPte;
		newPte.raw = physMem.Read32(pteAddr_write);
		REQUIRE(newPte.IsPresent());
		REQUIRE(newPte.IsWritable());
		REQUIRE(newPte.IsDirty());
		REQUIRE(os.GetPageFaultCount() == 1);
	}

	SECTION("Fault on non-present PDE leads to PT creation")
	{
		uint32_t large_vpn = 1024;
		uint32_t virtualAddress = large_vpn * PTE::PAGE_SIZE;
		uint32_t pdeAddr_for_vpn = 1 * PTE::PAGE_SIZE + 1 * sizeof(PTE);

		REQUIRE(physMem.Read32(pdeAddr_for_vpn) == 0);

		REQUIRE_NOTHROW(virtMem.Read8(virtualAddress, Privilege::User, false));

		PTE newPde;
		newPde.raw = physMem.Read32(pdeAddr_for_vpn);
		REQUIRE(newPde.IsPresent());
		REQUIRE(newPde.GetFrame() == 1000);

		uint32_t pt_addr = 1000 * PTE::PAGE_SIZE;
		uint32_t pte_addr = pt_addr + (large_vpn & 0x3FF) * sizeof(PTE);
		PTE newPte;
		newPte.raw = physMem.Read32(pte_addr);
		REQUIRE(newPte.IsPresent());
		REQUIRE(newPte.GetFrame() >= 3);
	}
}

TEST_CASE("MyOS Aging and Frame Displacement", "[MyOS][Aging]")
{
	PhysicalMemoryConfig config{ 8, 4096 };
	PhysicalMemory physMem(config);
	MyOS os(std::filesystem::temp_directory_path() / "os_aging.bin", physMem);
	VirtualMemory virtMem(physMem, os);

	SetupInitialMapping(physMem, virtMem, 0, 1);

	for (int i = 2; i < 8; ++i)
	{
		uint32_t vpn = i;
		uint32_t virtualAddress = vpn * PTE::PAGE_SIZE;
		uint32_t pteAddr = 1 * PTE::PAGE_SIZE + vpn * sizeof(PTE);

		physMem.Write32(pteAddr, 0);
		REQUIRE_NOTHROW(virtMem.Write8(virtualAddress, 0x01, Privilege::User));
	}

	REQUIRE(os.GetPageFaultCount() == 6);
	REQUIRE(os.GetSwapWriteCount() == 0);

	for (int i = 0; i < 10; ++i)
	{
		os.TimerInterrupt();
	}

	uint32_t vpn_victim = 2;
	uint32_t vpn_new = 8;
	uint32_t pteAddr_new = 1 * PTE::PAGE_SIZE + vpn_new * sizeof(PTE);
	physMem.Write32(pteAddr_new, 0);

	REQUIRE_NOTHROW(virtMem.Read8(vpn_new * PTE::PAGE_SIZE, Privilege::User, false));

	REQUIRE(os.GetPageFaultCount() == 7);
	REQUIRE(os.GetSwapWriteCount() >= 1);

	uint32_t pteAddr_victim = 1 * PTE::PAGE_SIZE + vpn_victim * sizeof(PTE);
	PTE victimPte;
	victimPte.raw = physMem.Read32(pteAddr_victim);
	REQUIRE_FALSE(victimPte.IsPresent());
}

TEST_CASE("MyOS Swap Operations (Read/Write)", "[MyOS][Swap]")
{
	PhysicalMemoryConfig config{ 4, 4096 };
	PhysicalMemory physMem(config);
	std::filesystem::path swapPath = std::filesystem::temp_directory_path() / "os_swap_full.bin";
	if (std::filesystem::exists(swapPath))
	{
		std::filesystem::remove(swapPath);
	}
	MyOS os(swapPath, physMem);
	VirtualMemory virtMem(physMem, os);

	SetupInitialMapping(physMem, virtMem, 0, 1);

	uint32_t vpn_dirty = 3;
	uint32_t vpn_dirty_addr = vpn_dirty * PTE::PAGE_SIZE;
	uint32_t pteAddr_dirty = 1 * PTE::PAGE_SIZE + vpn_dirty * sizeof(PTE);
	physMem.Write32(pteAddr_dirty, 0);
	REQUIRE_NOTHROW(virtMem.Write32(vpn_dirty_addr, 0xDEADBEEF, Privilege::User));

	uint32_t vpn_new = 4;
	uint32_t pteAddr_new = 1 * PTE::PAGE_SIZE + vpn_new * sizeof(PTE);
	physMem.Write32(pteAddr_new, 0);

	REQUIRE_NOTHROW(virtMem.Read8(vpn_new * PTE::PAGE_SIZE, Privilege::User, false));

	REQUIRE(os.GetSwapWriteCount() == 1);
	REQUIRE_FALSE(physMem.Read32(pteAddr_dirty) & PTE::P);

	uint32_t vpn_next = 5;
	uint32_t pteAddr_next = 1 * PTE::PAGE_SIZE + vpn_next * sizeof(PTE);
	physMem.Write32(pteAddr_next, 0);

	REQUIRE_NOTHROW(virtMem.Read8(vpn_next * PTE::PAGE_SIZE, Privilege::User, false));
	REQUIRE(os.GetSwapWriteCount() == 1);

	physMem.Write32(pteAddr_dirty, 0);

	REQUIRE_NOTHROW(virtMem.Read32(vpn_dirty_addr, Privilege::User, false));

	REQUIRE(os.GetSwapReadCount() == 1);

	PTE finalPte;
	finalPte.raw = physMem.Read32(pteAddr_dirty);
	uint32_t finalFrame = finalPte.GetFrame();
	uint32_t restoredValue = physMem.Read32(finalFrame * PTE::PAGE_SIZE);

	REQUIRE(restoredValue == 0xDEADBEEF);
}

TEST_CASE("MyOS WriteToReadOnly Fault (Unsupported)", "[MyOS][Fault]")
{
	PhysicalMemoryConfig config{ 10, 4096 };
	PhysicalMemory physMem(config);
	MyOS os(std::filesystem::temp_directory_path() / "os_fault_unsupported.bin", physMem);
	VirtualMemory virtMem(physMem, os);

	SetupInitialMapping(physMem, virtMem, 1, 2);

	SECTION("MisalignedAccess returns false")
	{
		bool result = os.OnPageFault(virtMem, 0x1000, Access::Read, PageFaultReason::MisalignedAccess);
		REQUIRE(result == false);
		REQUIRE(os.GetPageFaultCount() == 1);
	}

	SECTION("WriteToReadOnly returns false")
	{
		bool result = os.OnPageFault(virtMem, 0x1000, Access::Write, PageFaultReason::WriteToReadOnly);
		REQUIRE(result == false);
		REQUIRE(os.GetPageFaultCount() == 2);
	}
}