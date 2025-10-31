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
}