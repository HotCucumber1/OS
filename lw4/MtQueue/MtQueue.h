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
		m_cv_not_full.notify_one();
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
		m_cv_not_full.notify_one();
		return out;
	}

	T WaitAndPop()
	{
		static_assert(std::is_nothrow_move_constructible_v<T>, "WaitAndPop by value requires T to be nothrow move constructible");

		std::unique_lock lock(m_mutex);
		m_cv_not_empty.wait(lock, [this] {
			return !m_queue.empty();
		});

		T out = std::move(m_queue.front());

		m_queue.pop_front();
		m_cv_not_full.notify_one();
		return out;
	}

	void WaitAndPop(T& out)
	{
		std::unique_lock lock(m_mutex);
		m_cv_not_empty.wait(lock, [this] {
			return !m_queue.empty();
		});

		out = std::move(m_queue.front());

		m_queue.pop_front();
		m_cv_not_full.notify_one();
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

	void Swap(MtQueue& other)
	{
		if (this == &other)
		{
			return;
		}

		std::scoped_lock lock(m_mutex, other.m_mutex);
		std::swap(m_queue, other.m_queue);
		std::swap(m_capacity, other.m_capacity);

		if (!IsEmpty())
		{
			m_cv_not_empty.notify_all();
		}
		if (!other.IsEmpty())
		{
			other.m_cv_not_empty.notify_all();
		}

		if (!IsFull())
		{
			m_cv_not_full.notify_all();
		}
		if (!other.IsFull())
		{
			m_cv_not_full.notify_all();
		}
	}

	void Swap(std::deque<T>& other)
	{
		std::unique_lock lock(m_mutex);
		std::swap(m_queue, other);

		if (!IsEmpty())
		{
			m_cv_not_empty.notify_all();
		}
		if (!IsFull())
		{
			m_cv_not_full.notify_all();
		}
	}

private:
	bool IsFull()
	{
		return m_capacity > 0 && m_queue.size() == m_capacity;
	}

	template<typename U>
	void DoPush(U&& value)
	{
		std::unique_lock lock(m_mutex);

		if (m_capacity > 0)
		{
			m_cv_not_full.wait(lock, [this] {
				return !IsFull();
			});
		}

		m_queue.emplace_back(std::forward<U>(value));
		m_cv_not_empty.notify_one();
	}

	template<typename U>
	bool DoTryPush(U&& value)
	{
		std::unique_lock lock(m_mutex);
		if (IsFull())
		{
			return false;
		}

		m_queue.emplace_back(std::forward<U>(value));
		m_cv_not_empty.notify_one();
		return true;
	}

	size_t m_capacity;
	std::deque<T> m_queue;
	mutable std::shared_mutex m_mutex;
	std::condition_variable_any m_cv_not_empty;
	std::condition_variable_any m_cv_not_full;
};