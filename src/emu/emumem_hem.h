// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
#ifndef MAME_EMU_EMUMEM_HEM_H
#define MAME_EMU_EMUMEM_HEM_H

#pragma once

// handler_entry_read_memory/handler_entry_write_memory

// Accesses fixed memory (non-banked rom or ram)

template<int Width, int AddrShift> class handler_entry_read_memory : public handler_entry_read_address<Width, AddrShift>
{
public:
	using uX = typename emu::detail::handler_entry_size<Width>::uX;

	handler_entry_read_memory(address_space *space, u16 flags, void *base) : handler_entry_read_address<Width, AddrShift>(space, flags), m_base(reinterpret_cast<uX *>(base)) {}
	~handler_entry_read_memory() = default;

	uX read(offs_t offset, uX mem_mask) const override;
	uX read_interruptible(offs_t offset, uX mem_mask) const override;
	std::pair<uX, u16> read_flags(offs_t offset, uX mem_mask) const override;
	u16 lookup_flags(offs_t offset, uX mem_mask) const override;
	void *get_ptr(offs_t offset) const override;

	std::string name() const override;

private:
	uX *m_base;
};

template<int Width, int AddrShift> class handler_entry_write_memory : public handler_entry_write_address<Width, AddrShift>
{
public:
	using uX = typename emu::detail::handler_entry_size<Width>::uX;

	handler_entry_write_memory(address_space *space, u16 flags, void *base) : handler_entry_write_address<Width, AddrShift>(space, flags), m_base(reinterpret_cast<uX *>(base)) {}
	~handler_entry_write_memory() = default;

	void write(offs_t offset, uX data, uX mem_mask) const override;
	void write_interruptible(offs_t offset, uX data, uX mem_mask) const override;
	u16 write_flags(offs_t offset, uX data, uX mem_mask) const override;
	u16 lookup_flags(offs_t offset, uX mem_mask) const override;
	void *get_ptr(offs_t offset) const override;

	std::string name() const override;

private:
	uX *m_base;
};


// handler_entry_read_memory_bank/handler_entry_write_memory_bank

// Accesses banked memory, associated to a memory_bank

template<int Width, int AddrShift> class handler_entry_read_memory_bank : public handler_entry_read_address<Width, AddrShift>
{
public:
	using uX = typename emu::detail::handler_entry_size<Width>::uX;

	handler_entry_read_memory_bank(address_space *space, u16 flags, memory_bank &bank) : handler_entry_read_address<Width, AddrShift>(space, flags), m_bank(bank) {}
	~handler_entry_read_memory_bank() = default;

	uX read(offs_t offset, uX mem_mask) const override;
	uX read_interruptible(offs_t offset, uX mem_mask) const override;
	std::pair<uX, u16> read_flags(offs_t offset, uX mem_mask) const override;
	u16 lookup_flags(offs_t offset, uX mem_mask) const override;
	void *get_ptr(offs_t offset) const override;

	std::string name() const override;

private:
	memory_bank &m_bank;
};

template<int Width, int AddrShift> class handler_entry_write_memory_bank : public handler_entry_write_address<Width, AddrShift>
{
public:
	using uX = typename emu::detail::handler_entry_size<Width>::uX;

	handler_entry_write_memory_bank(address_space *space, u16 flags, memory_bank &bank) : handler_entry_write_address<Width, AddrShift>(space, flags), m_bank(bank) {}
	~handler_entry_write_memory_bank() = default;

	void write(offs_t offset, uX data, uX mem_mask) const override;
	void write_interruptible(offs_t offset, uX data, uX mem_mask) const override;
	u16 write_flags(offs_t offset, uX data, uX mem_mask) const override;
	u16 lookup_flags(offs_t offset, uX mem_mask) const override;
	void *get_ptr(offs_t offset) const override;

	std::string name() const override;

private:
	memory_bank &m_bank;
};

#endif // MAME_EMU_EMUMEM_HEM_H
