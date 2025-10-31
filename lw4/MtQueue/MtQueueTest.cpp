#include "MtQueue.h"
#include <catch2/catch_all.hpp>
#include <utility>

struct QueueItem
{
	int id;
	std::string name;
};

class Bomb
{
public:
	static inline bool s_explodeOnCopy = false;
	static inline bool s_explodeOnMove = false;

	int value;

	explicit Bomb(const int v = 0)
		: value(v)
	{
	}

	Bomb(const Bomb& other)
		: value(other.value)
	{
		if (s_explodeOnCopy)
		{
			throw std::runtime_error("Copy Boom!");
		}
	}

	Bomb(Bomb&& other)
		: value(other.value)
	{
		if (s_explodeOnMove)
		{
			throw std::runtime_error("Boom on move!");
		}
	}
};

TEST_CASE("Single thread")
{
	SECTION("Try pop")
	{
		MtQueue<QueueItem> queue;

		REQUIRE(queue.IsEmpty());
		QueueItem out;
		REQUIRE_FALSE(queue.TryPop(out));
		REQUIRE(queue.TryPop() == nullptr);
	}

	SECTION("Bounded")
	{
		MtQueue<QueueItem> queue(1);
		REQUIRE(queue.TryPush({ 1, "hello" }));
		REQUIRE_FALSE(queue.TryPush({ 2, "world" }));
	}

	SECTION("Unbounded")
	{
		MtQueue<QueueItem> queue;
		REQUIRE(queue.TryPush({ 1, "hello" }));
		REQUIRE(queue.TryPush({ 2, "hello" }));
		REQUIRE(queue.TryPush({ 3, "hello" }));

		REQUIRE(queue.TryPop()->id == 1);
		REQUIRE(queue.TryPop()->id == 2);
		REQUIRE(queue.TryPop()->id == 3);
		REQUIRE(queue.TryPop() == nullptr);
	}

	SECTION("Copy exception")
	{
		Bomb::s_explodeOnCopy = true;

		MtQueue<Bomb> queue(2);

		Bomb::s_explodeOnCopy = false;
		queue.Push(Bomb(1));

		Bomb::s_explodeOnCopy = true;
		Bomb bomb(2);

		REQUIRE_THROWS(queue.Push(bomb));
		REQUIRE(queue.GetSize() == 1);

		Bomb::s_explodeOnCopy = false;
		queue.Push(Bomb(3));
		REQUIRE(queue.GetSize() == 2);
	}

	SECTION("Move exception")
	{
		MtQueue<Bomb> queue;
		Bomb::s_explodeOnMove = true;

		REQUIRE_THROWS(queue.Push(Bomb(1)));
		REQUIRE(queue.IsEmpty());

		Bomb::s_explodeOnMove = false;
		queue.Push(Bomb(2));
		REQUIRE(queue.GetSize() == 1);
	}

	SECTION("Swap non-empty queue with empty deque")
	{
		MtQueue<std::string> queue(3);
		queue.Push("hello");
		queue.Push("world");

		std::deque<std::string> externalDeque;

		REQUIRE(queue.GetSize() == 2);
		REQUIRE(externalDeque.empty() == true);

		queue.Swap(externalDeque);

		REQUIRE(queue.IsEmpty() == true);
		REQUIRE(externalDeque.size() == 2);
		REQUIRE(externalDeque.front() == "hello");
		REQUIRE(externalDeque.back() == "world");
	}
}

TEST_CASE("Multiple threads")
{
	SECTION("Push")
	{
		MtQueue<QueueItem> queue(1);
		std::atomic push_started{ false };
		std::atomic push_completed{ false };

		queue.Push({ 1, "hello" });
		REQUIRE(queue.GetSize() == 1);

		std::thread producer([&] {
			push_started = true;
			queue.Push({ 2, "world" });
			push_completed = true;
		});

		while (!push_started)
		{
			std::this_thread::yield();
		}

		REQUIRE(push_completed == false);
		REQUIRE(queue.GetSize() == 1);

		QueueItem value;
		REQUIRE(queue.TryPop(value) == true);
		REQUIRE(value.id == 1);

		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		REQUIRE(push_completed == true);
		REQUIRE(queue.GetSize() == 1);
		REQUIRE(queue.TryPop(value) == true);
		REQUIRE(value.id == 2);

		producer.join();
	}

	SECTION("Producer-consumer")
	{
		constexpr int N_PRODUCERS = 3;
		constexpr int M_CONSUMERS = 2;
		constexpr int K_ELEMENTS_PER_PRODUCER = 10;
		constexpr int TOTAL_ELEMENTS = N_PRODUCERS * K_ELEMENTS_PER_PRODUCER;

		MtQueue<int> queue(10);
		std::atomic producedCount{ 0 };
		std::atomic consumedCount{ 0 };
		std::vector<int> consumedElements;
		std::mutex mutex;

		std::vector<std::thread> producers;
		for (int i = 0; i < N_PRODUCERS; ++i)
		{
			producers.emplace_back([&, i] {
				for (int j = 0; j < K_ELEMENTS_PER_PRODUCER; ++j)
				{
					int element = i * K_ELEMENTS_PER_PRODUCER + j;
					queue.Push(element);
					producedCount.fetch_add(1, std::memory_order_relaxed);
				}
			});
		}

		std::vector<std::thread> consumers;
		for (int i = 0; i < M_CONSUMERS; ++i)
		{
			consumers.emplace_back([&] {
				while (consumedCount.load(std::memory_order_relaxed) < TOTAL_ELEMENTS)
				{
					auto element = queue.TryPop();
					if (element)
					{
						std::lock_guard lock(mutex);
						consumedElements.push_back(*element);
						consumedCount.fetch_add(1, std::memory_order_relaxed);
					}
					else
					{
						std::this_thread::yield();
					}
				}
			});
		}

		for (auto& producer : producers)
		{
			producer.join();
		}

		while (consumedCount.load() < TOTAL_ELEMENTS)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		for (auto& consumer : consumers)
		{
			consumer.join();
		}

		REQUIRE(producedCount == TOTAL_ELEMENTS);
		REQUIRE(consumedCount == TOTAL_ELEMENTS);
		REQUIRE(consumedElements.size() == TOTAL_ELEMENTS);
		REQUIRE(queue.IsEmpty());
	}

	SECTION("Swap")
	{
		constexpr int SWAP_COUNT = 100;
		MtQueue<int> queue1(3);
		MtQueue<int> queue2(5);

		queue1.Push(1);
		queue1.Push(2);
		queue1.Push(3);

		queue2.Push(10);
		queue2.Push(20);

		std::atomic thread1Done{ false };
		std::atomic thread2Done{ false };
		std::atomic swapCount{ 0 };

		std::thread thread1([&] {
			for (int i = 0; i < SWAP_COUNT; ++i)
			{
				queue1.Swap(queue2);
				swapCount.fetch_add(1);
			}
			thread1Done = true;
		});

		std::thread thread2([&] {
			for (int i = 0; i < SWAP_COUNT; ++i)
			{
				queue2.Swap(queue1);
				swapCount.fetch_add(1);
			}
			thread2Done = true;
		});

		thread1.join();
		thread2.join();

		REQUIRE(thread1Done == true);
		REQUIRE(thread2Done == true);
		REQUIRE(swapCount == 200);

		REQUIRE(queue1.GetSize() + queue2.GetSize() == 5);
	}

	SECTION("Notifications after swap")
	{
		MtQueue<int> queue(3);
		std::deque externalDeque = { 42, 84 };

		std::atomic popCompleted{ false };
		std::thread consumer([&] {
			const int value = queue.WaitAndPop();
			REQUIRE(value == 42);
			popCompleted = true;
		});

		REQUIRE(popCompleted == false);
		queue.Swap(externalDeque);

		consumer.join();
		REQUIRE(popCompleted == true);
		int value;
		REQUIRE(queue.TryPop(value) == true);
		REQUIRE(value == 84);
	}
}