#include "MtQueue.h"
#include <atomic>
#include <catch2/catch_all.hpp>
#include <iostream>
#include <thread>
#include <vector>

TEST_CASE("WaitAndPop notification benchmark")
{
	constexpr double MS_IN_S = 1000000000;
	constexpr int NUM_CONSUMERS = 8;
	constexpr int NUM_ELEMENTS = 100;
	constexpr int SWAP_COUNT = 10000;

	BENCHMARK("notify_all - multy waiting")
	{
		MtQueue<int> sourceQueue(NUM_ELEMENTS);
		MtQueue<int> emptyQueue(NUM_ELEMENTS);

		std::atomic stop{ false };
		std::atomic consumedCount{ 0 };

		for (int i = 0; i < NUM_ELEMENTS; ++i)
		{
			sourceQueue.Push(i);
		}

		std::vector<std::jthread> consumers;
		for (int i = 0; i < NUM_CONSUMERS; ++i)
		{
			consumers.emplace_back([&] {
				while (!stop)
				{
					int value = emptyQueue.WaitAndPop();
					consumedCount.fetch_add(1);
				}
			});
		}

		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < SWAP_COUNT; ++i)
		{
			emptyQueue.Swap(sourceQueue, false);
			sourceQueue.Swap(emptyQueue, false);
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto swapTime = (end - start) / MS_IN_S;

		std::cout << "Swap: " << swapTime << std::endl;

		stop = true;
		emptyQueue.Shutdown();
		sourceQueue.Shutdown();

		return consumedCount.load();
	};

	BENCHMARK("notify_one - multy waiting")
	{
		MtQueue<int> sourceQueue(NUM_ELEMENTS);
		MtQueue<int> emptyQueue(NUM_ELEMENTS);

		std::atomic stop{ false };
		std::atomic consumedCount{ 0 };

		for (int i = 0; i < NUM_ELEMENTS; ++i)
		{
			sourceQueue.Push(i);
		}

		std::vector<std::jthread> consumers;
		for (int i = 0; i < NUM_CONSUMERS; ++i)
		{
			consumers.emplace_back([&] {
				while (!stop)
				{
					int value = emptyQueue.WaitAndPop();
					consumedCount.fetch_add(1);
				}
			});
		}

		for (int i = 0; i < SWAP_COUNT; ++i)
		{
			emptyQueue.Swap(sourceQueue, false);
			sourceQueue.Swap(emptyQueue, false);
		}

		stop = true;
		emptyQueue.Shutdown();
		sourceQueue.Shutdown();

		return consumedCount.load();
	};

	BENCHMARK("notify_all - one waiting")
	{
		MtQueue<int> sourceQueue(NUM_ELEMENTS);
		MtQueue<int> emptyQueue(NUM_ELEMENTS);

		std::atomic stop{ false };
		std::atomic consumedCount{ 0 };

		for (int i = 0; i < NUM_ELEMENTS; ++i)
		{
			sourceQueue.Push(i);
		}

		std::vector<std::jthread> consumers;
		for (int i = 0; i < 1; ++i)
		{
			consumers.emplace_back([&] {
				while (!stop)
				{
					int value = emptyQueue.WaitAndPop();
					consumedCount.fetch_add(1);
				}
			});
		}

		for (int i = 0; i < SWAP_COUNT; ++i)
		{
			emptyQueue.Swap(sourceQueue, true);
			sourceQueue.Swap(emptyQueue, true);
		}

		stop = true;
		emptyQueue.Shutdown();
		sourceQueue.Shutdown();

		return consumedCount.load();
	};

	BENCHMARK("notify_one - one waiting")
	{
		MtQueue<int> sourceQueue(NUM_ELEMENTS);
		MtQueue<int> emptyQueue(NUM_ELEMENTS);

		std::atomic stop{ false };
		std::atomic consumedCount{ 0 };

		for (int i = 0; i < NUM_ELEMENTS; ++i)
		{
			sourceQueue.Push(i);
		}

		std::vector<std::jthread> consumers;
		for (int i = 0; i < 1; ++i)
		{
			consumers.emplace_back([&] {
				while (!stop)
				{
					int value = emptyQueue.WaitAndPop();
					consumedCount.fetch_add(1);
				}
			});
		}

		for (int i = 0; i < SWAP_COUNT; ++i)
		{
			emptyQueue.Swap(sourceQueue, true);
			sourceQueue.Swap(emptyQueue, true);
		}

		stop = true;
		emptyQueue.Shutdown();
		sourceQueue.Shutdown();

		return consumedCount.load();
	};
}