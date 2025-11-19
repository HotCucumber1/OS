#pragma once
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename T>
class MtQueue
{
public:
	explicit MtQueue(const size_t capacity = 0)
		: m_capacity(capacity)
	{
	}

	void Push(const T& value)
	{
		DoPush(value);
	}

	void Push(T&& value)
	{
		DoPush(std::move(value));
	}

	[[nodiscard]] bool TryPush(const T& value)
	{
		return DoTryPush(value);
	}

	[[nodiscard]] bool TryPush(T&& value)
	{
		return DoTryPush(std::move(value));
	}

	bool TryPop(T& out)
	{
		std::unique_lock lock(m_mutex);
		if (m_queue.empty())
		{
			return false;
		}
		out = std::move(m_queue.front());
		m_queue.pop_front();
		m_cvNotFull.notify_one();
		return true;
	}

	std::unique_ptr<T> TryPop()
	{
		std::unique_lock lock(m_mutex);
		if (m_queue.empty())
		{
			return nullptr;
		}
		auto out = std::make_unique<T>(std::move(m_queue.front()));

		m_queue.pop_front();
		m_cvNotFull.notify_one();
		return out;
	}

	T WaitAndPop()
	{
		static_assert(std::is_nothrow_move_constructible_v<T>, "WaitAndPop by value requires T to be nothrow move constructible");

		std::unique_lock lock(m_mutex);
		m_cvNotEmpty.wait(lock, [this] {
			return !m_queue.empty() || m_shutDown;
		});

		T out = std::move(m_queue.front());

		m_queue.pop_front();
		m_cvNotFull.notify_one();
		return out;
	}

	void WaitAndPop(T& out)
	{
		std::unique_lock lock(m_mutex);
		m_cvNotEmpty.wait(lock, [this] {
			return !m_queue.empty() || m_shutDown;
		});

		if constexpr (std::is_nothrow_move_assignable_v<T> || !std::is_copy_assignable_v<T>)
		{
			out = std::move(m_queue.front());
		}
		else
		{
			out = m_queue.front();
		}

		m_queue.pop_front();
		m_cvNotFull.notify_one();
	}

	size_t GetSize() const
	{
		std::shared_lock lock(m_mutex);
		return m_queue.size();
	}

	[[nodiscard]] bool IsEmpty() const
	{
		std::shared_lock lock(m_mutex);
		return m_queue.empty();
	}

	void Swap(MtQueue& other, const bool needNotifyAll = true)
	{
		if (this == &other)
		{
			return;
		}

		std::scoped_lock lock(m_mutex, other.m_mutex);
		DoSwap(other, needNotifyAll);
	}

	void Swap(std::deque<T>& other)
	{
		std::unique_lock lock(m_mutex);
		std::swap(m_queue, other);

		if (!m_queue.empty())
		{
			m_cvNotEmpty.notify_all();
		}
		if (!IsFull())
		{
			m_cvNotFull.notify_all();
		}
	}

	void Shutdown()
	{
		std::lock_guard lock(m_mutex);
		m_shutDown = true;
		m_cvNotEmpty.notify_all();
		m_cvNotFull.notify_all();
	}

private:
	bool IsFull() const
	{
		return m_capacity > 0 && m_queue.size() == m_capacity;
	}

	template <typename U>
	void DoPush(U&& value)
	{
		std::unique_lock lock(m_mutex);

		if (m_capacity > 0)
		{
			m_cvNotFull.wait(lock, [this] {
				return !IsFull();
			});
		}

		m_queue.emplace_back(std::forward<U>(value));
		m_cvNotEmpty.notify_one();
	}

	template <typename U>
	bool DoTryPush(U&& value)
	{
		std::unique_lock lock(m_mutex);
		if (IsFull())
		{
			return false;
		}

		m_queue.emplace_back(std::forward<U>(value));
		m_cvNotEmpty.notify_one();
		return true;
	}

	void DoSwap(MtQueue& other, const bool needNotifyAll)
	{
		std::swap(m_queue, other.m_queue);
		std::swap(m_capacity, other.m_capacity);
		std::swap(m_shutDown, other.m_shutDown);

		auto notify = [needNotifyAll](auto& cv) {
			if (needNotifyAll)
			{
				cv.notify_all();
			}
			else
			{
				cv.notify_one();
			}
		};

		if (!m_queue.empty())
		{
			notify(m_cvNotEmpty);
		}
		if (!other.m_queue.empty())
		{
			notify(other.m_cvNotEmpty);
		}
		if (!IsFull())
		{
			notify(m_cvNotFull);
		}
		if (!other.IsFull())
		{
			notify(other.m_cvNotFull);
		}
	}

	bool m_shutDown = false;
	size_t m_capacity;
	std::deque<T> m_queue;
	mutable std::shared_mutex m_mutex;
	std::condition_variable_any m_cvNotEmpty;
	std::condition_variable_any m_cvNotFull;
};