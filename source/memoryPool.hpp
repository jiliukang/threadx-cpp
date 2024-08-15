#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <array>

namespace ThreadX
{
class BytePoolBase : protected Native::TX_BYTE_POOL
{
  public:
    template <class Pool> friend class Allocation;
    BytePoolBase(const BytePoolBase &) = delete;
    BytePoolBase &operator=(const BytePoolBase &) = delete;

    static constexpr auto minimumPoolSize(std::span<const Ulong> memorySizes);

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();
    std::string_view name() const;

  protected:
    explicit BytePoolBase();
    ///
    ~BytePoolBase();
};

constexpr auto BytePoolBase::minimumPoolSize(std::span<const Ulong> memorySizes)
{
    Ulong poolSize{2 * sizeof(uintptr_t)};
    for (auto memSize : memorySizes)
    {
        poolSize += (memSize + 2 * sizeof(uintptr_t));
    }

    return poolSize;
}

/// byte memory pool from which to allocate the thread stacks and queues.
/// \tparam Size size of byte pool in bytes
template <Ulong Size> class BytePool : public BytePoolBase
{
    static_assert(Size % wordSize == 0, "Pool size must be a multiple of word size.");

  public:
    ///
    explicit BytePool(const std::string_view name);

  private:
    std::array<Ulong, Size / wordSize> m_pool{}; // Ulong alignment
};

template <Ulong Size> BytePool<Size>::BytePool(const std::string_view name)
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_byte_pool_create(this, const_cast<char *>(name.data()), m_pool.data(), Size)};
    assert(error == Error::success);
}

class BlockPoolBase : protected Native::TX_BLOCK_POOL
{
  public:
    template <class Pool> friend class Allocation;
    BlockPoolBase(const BlockPoolBase &) = delete;
    BlockPoolBase &operator=(const BlockPoolBase &) = delete;

    Ulong blockSize() const;

    /// Places the highest priority thread suspended for memory on this pool at the front of the suspension list.
    /// All other threads remain in the same FIFO order they were suspended in.
    Error prioritise();
    std::string_view name() const;

  protected:
    explicit BlockPoolBase();
    ///
    ~BlockPoolBase();
};

template <Ulong Size, Ulong BlockSize> class BlockPool : public BlockPoolBase
{
    static_assert(Size % wordSize == 0, "Pool size must be a multiple of word size.");
    static_assert(Size % (BlockSize + sizeof(void *)) == 0);

  public:
    /// block memory pool from which to allocate the thread stacks and queues.
    /// total blocks = (total bytes) / (block size + sizeof(void *))
    explicit BlockPool(const std::string_view name);

  private:
    std::array<Ulong, Size / wordSize> m_pool{}; // Ulong alignment
};

template <Ulong Size, Ulong BlockSize> BlockPool<Size, BlockSize>::BlockPool(const std::string_view name)
{
    using namespace Native;
    [[maybe_unused]] Error error{
        tx_block_pool_create(this, const_cast<char *>(name.data()), BlockSize, m_pool.data(), Size)};
    assert(error == Error::success);
}

template <class Pool> class Allocation
{
  public:
    Allocation(const Allocation &) = delete;
    Allocation &operator=(const Allocation &) = delete;

    template <typename Rep = TickTimer::rep, typename Period = TickTimer::period>
    Allocation(Pool &pool, const Ulong memorySizeInBytes,
               const std::chrono::duration<Rep, Period> &duration = TickTimer::noWait)
        requires(std::is_base_of_v<BytePoolBase, Pool>)
    {
        [[maybe_unused]] Error error{tx_byte_allocate(
            std::addressof(pool), std::addressof(memoryPtr), memorySizeInBytes,
            TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration)))};
        assert(error == Error::success);
    }

    template <typename Rep = TickTimer::rep, typename Period = TickTimer::period>
    Allocation(Pool &pool, const std::chrono::duration<Rep, Period> &duration = TickTimer::noWait)
        requires(std::is_base_of_v<BlockPoolBase, Pool>)
    {
        [[maybe_unused]] Error error{tx_block_allocate(
            tx_byte_allocate(std::addressof(pool), std::addressof(memoryPtr),
                             TickTimer::ticks(std::chrono::duration_cast<TickTimer::Duration>(duration))))};
        assert(error == Error::success);
    }

    void *getPtr()
    {
        return memoryPtr;
    }

    ~Allocation()
        requires(std::is_base_of_v<BytePoolBase, Pool>)
    {
        [[maybe_unused]] Error error{Native::tx_byte_release(memoryPtr)};
        assert(error == Error::success);
    }

    ~Allocation()
        requires(std::is_base_of_v<BlockPoolBase, Pool>)
    {
        [[maybe_unused]] Error error{Native::tx_block_release(memoryPtr)};
        assert(error == Error::success);
    }

  private:
    void *memoryPtr{};
};
} // namespace ThreadX
