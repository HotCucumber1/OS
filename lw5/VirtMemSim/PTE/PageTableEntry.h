#pragma once
#include <cstdint>

// 4-КБ страницы, 32-битный PTE (учебная модель)
struct PTE // запись (entry) с информацией о каждой странице
{
	uint32_t raw = 0;

	static constexpr uint32_t P = 1u << 0; // Present
	static constexpr uint32_t RW = 1u << 1; // 1 = Read/Write, 0 = Read-Only
	static constexpr uint32_t US = 1u << 2; // 1 = User, 0 = Supervisor
	static constexpr uint32_t A = 1u << 5; // Accessed (используем как R)
	static constexpr uint32_t D = 1u << 6; // Dirty (используем как M)
	static constexpr uint32_t NX = 1u << 31; // No-Execute (учебное поле)

	static constexpr uint32_t FRAME_SHIFT = 12;
	static constexpr uint32_t FRAME_MASK = 0xFFFFF000u;
	static constexpr uint32_t PAGE_SIZE = 4096;

	uint32_t GetFrame() const // frame - номер физической ячейки с данными
	{
		return (raw & FRAME_MASK) >> FRAME_SHIFT;
	}

	void SetFrame(const uint32_t fn)
	{
		raw = (raw & ~FRAME_MASK) | (fn << FRAME_SHIFT);
	}

	bool IsPresent() const
	{
		return raw & P;
	}

	void SetPresent(const bool v)
	{
		raw = v
			? (raw | P)
			: (raw & ~P);
	}

	bool IsWritable() const
	{
		return raw & RW;
	}

	void SetWritable(const bool v)
	{
		raw = v
			? (raw | RW)
			: (raw & ~RW);
	}

	bool IsUser() const
	{
		return raw & US;
	}
	void SetUser(const bool v)
	{
		raw = v
			? (raw | US)
			: (raw & ~US);
	}

	bool IsAccessed() const
	{
		return raw & A;
	}

	void SetAccessed(const bool v)
	{
		raw = v
			? (raw | A)
			: (raw & ~A);
	}

	bool IsDirty() const
	{
		return raw & D;
	}

	void SetDirty(const bool v)
	{
		raw = v
			? (raw | D)
			: (raw & ~D);
	}

	bool IsNX() const
	{
		return raw & NX;
	}

	void SetNX(const bool v)
	{
		raw = v
			? (raw | NX)
			: (raw & ~NX);
	}
};