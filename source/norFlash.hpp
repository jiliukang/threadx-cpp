#pragma once

#include "lxCommon.hpp"
#include <array>
#include <atomic>
#include <functional>
#include <span>
#include <utility>

namespace LevelX
{
static constexpr ThreadX::Uint norSectorSizeInWord{LX_NATIVE_NOR_SECTOR_SIZE};

class NorFlashBase
{
  protected:
    static inline std::atomic_flag m_initialised = ATOMIC_FLAG_INIT;
};

template <ThreadX::Ulong CacheSize = 0> class NorFlash : protected ThreadX::Native::LX_NOR_FLASH, NorFlashBase
{
    static_assert(CacheSize % (norSectorSizeInWord * ThreadX::wordSize) == 0);

  public:
    static constexpr auto sectorSize();

    explicit NorFlash(
        const ThreadX::Ulong storageSize, const ThreadX::Ulong blockSize, const ThreadX::Ulong baseAddress = 0);

    auto formatSize() const;
    auto open();
    auto close();
    auto defragment();
    auto defragment(const ThreadX::Uint numberOfBlocks);
    auto readSector(const ThreadX::Ulong sectorNumber, std::span<ThreadX::Ulong, norSectorSizeInWord> sectorData);
    auto readSector(const ThreadX::Ulong sectorNumber, ThreadX::Uchar *const sectorDataPtr);
    auto releaseSector(const ThreadX::Ulong sectorNumber);
    auto writeSector(const ThreadX::Ulong sectorNumber, std::span<ThreadX::Ulong, norSectorSizeInWord> sectorData);
    auto writeSector(const ThreadX::Ulong sectorNumber, ThreadX::Uchar *const sectorDataPtr);

    virtual Error initialiseCallback();
    virtual Error readCallback(ThreadX::Ulong *flashAddress, ThreadX::Ulong *destination, ThreadX::Ulong words) = 0;
    //LevelX relies on the driver to verify that the write sector was successful. This is typically done by reading back the programmed value to ensure it matches the requested value to be written.
    virtual Error writeCallback(ThreadX::Ulong *flashAddress, ThreadX::Ulong *source, ThreadX::Ulong words) = 0;
    //LevelX relies on the driver to examine all bytes of the block to ensure they are erased (contain all ones).
    virtual Error eraseBlockCallback(ThreadX::Ulong block, ThreadX::Ulong eraseCount) = 0;
    //LevelX relies on the driver to examine all bytes of the specified to ensure they are erased (contain all ones).
    virtual Error verifyErasedBlockCallback(const ThreadX::Ulong block) = 0;
    virtual Error systemErrorCallback(const ThreadX::Uint errorCode);

  protected:
    virtual ~NorFlash();

  private:
    struct DriverCallback
    {
        static auto initialise(ThreadX::Native::LX_NOR_FLASH *norFlashPtr);
        static auto read(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                         ThreadX::Ulong *destination, ThreadX::Ulong words);
        static auto write(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                          ThreadX::Ulong *source, ThreadX::Ulong words);
        static auto eraseBlock(
            ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block, ThreadX::Ulong eraseCount);
        static auto verifyErasedBlock(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block);
        static auto systemError(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Uint errorCode);
    };

    const ThreadX::Ulong m_storageSize;
    const ThreadX::Ulong m_blockSize;
    const ThreadX::Ulong m_baseAddress;
    std::array<ThreadX::Ulong, norSectorSizeInWord> m_sectorBuffer{};
    std::array<ThreadX::Ulong, CacheSize / ThreadX::wordSize> m_extendedCacheMemory{};
};

template <ThreadX::Ulong CacheSize> constexpr auto NorFlash<CacheSize>::sectorSize()
{
    return FileX::MediaSectorSize{norSectorSizeInWord * ThreadX::wordSize};
}

template <ThreadX::Ulong CacheSize>
NorFlash<CacheSize>::NorFlash(
    const ThreadX::Ulong storageSize, const ThreadX::Ulong blockSize, const ThreadX::Ulong baseAddress)
    : ThreadX::Native::LX_NOR_FLASH{}, m_storageSize{storageSize}, m_blockSize{blockSize}, m_baseAddress{baseAddress}
{
    assert(storageSize % blockSize == 0);
    assert(blockSize % (norSectorSizeInWord * ThreadX::wordSize) == 0);
    assert(blockSize / (norSectorSizeInWord * ThreadX::wordSize) >= 2 and
           blockSize / (norSectorSizeInWord * ThreadX::wordSize) <= 120);

    if (not m_initialised.test_and_set())
    {
        ThreadX::Native::lx_nor_flash_initialize();
    }
}

template <ThreadX::Ulong CacheSize> NorFlash<CacheSize>::~NorFlash()
{
    [[maybe_unused]] Error error{lx_nor_flash_close(this)};
    assert(error == Error::success);
}

template <ThreadX::Ulong CacheSize> auto NorFlash<CacheSize>::formatSize() const
{
    return ThreadX::Ulong{(lx_nor_flash_total_blocks - 1) * (lx_nor_flash_words_per_block * ThreadX::wordSize)};
}

template <ThreadX::Ulong CacheSize> auto NorFlash<CacheSize>::open()
{
    Error error{lx_nor_flash_open(this, const_cast<char *>("nor flash"), DriverCallback::initialise)};
    if (error != Error::success)
    {
        return error;
    }

    if (auto cacheSize{m_extendedCacheMemory.size() * ThreadX::wordSize}; cacheSize > 0)
    {
        error = Error{lx_nor_flash_extended_cache_enable(this, m_extendedCacheMemory.data(), cacheSize)};
    }

    return error;
}

template <ThreadX::Ulong CacheSize> auto NorFlash<CacheSize>::close()
{
    return Error{lx_nor_flash_close(this)};
}

template <ThreadX::Ulong CacheSize> auto NorFlash<CacheSize>::defragment()
{
    return Error{lx_nor_flash_defragment(this)};
}

template <ThreadX::Ulong CacheSize> auto NorFlash<CacheSize>::defragment(const ThreadX::Uint numberOfBlocks)
{
    return Error{lx_nor_flash_partial_defragment(this, numberOfBlocks)};
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::readSector(
    const ThreadX::Ulong sectorNumber, std::span<ThreadX::Ulong, norSectorSizeInWord> sectorData)
{
    return Error{lx_nor_flash_sector_read(this, sectorNumber, sectorData.data())};
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::readSector(const ThreadX::Ulong sectorNumber, ThreadX::Uchar *const sectorDataPtr)
{
    return Error{lx_nor_flash_sector_read(this, sectorNumber, sectorDataPtr)};
}

template <ThreadX::Ulong CacheSize> auto NorFlash<CacheSize>::releaseSector(const ThreadX::Ulong sectorNumber)
{
    return Error{lx_nor_flash_sector_release(this, sectorNumber)};
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::writeSector(
    const ThreadX::Ulong sectorNumber, std::span<ThreadX::Ulong, norSectorSizeInWord> sectorData)
{
    return Error{lx_nor_flash_sector_write(this, sectorNumber, sectorData.data())};
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::writeSector(const ThreadX::Ulong sectorNumber, ThreadX::Uchar *const sectorDataPtr)
{
    return Error{lx_nor_flash_sector_write(this, sectorNumber, sectorDataPtr)};
}

template <ThreadX::Ulong CacheSize> Error NorFlash<CacheSize>::initialiseCallback()
{
    return Error::success;
}

template <ThreadX::Ulong CacheSize>
Error NorFlash<CacheSize>::systemErrorCallback([[maybe_unused]] const ThreadX::Uint errorCode)
{
    return Error::success;
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::DriverCallback::initialise(ThreadX::Native::LX_NOR_FLASH *norFlashPtr)
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};

    norFlash.lx_nor_flash_base_address = reinterpret_cast<ThreadX::Ulong *>(norFlash.m_baseAddress);
    norFlash.lx_nor_flash_total_blocks = norFlash.m_storageSize / norFlash.m_blockSize;
    norFlash.lx_nor_flash_words_per_block = norFlash.m_blockSize / ThreadX::wordSize;
    norFlash.lx_nor_flash_sector_buffer = norFlash.m_sectorBuffer.data();

    norFlash.lx_nor_flash_driver_read = DriverCallback::read;
    norFlash.lx_nor_flash_driver_write = DriverCallback::write;
    norFlash.lx_nor_flash_driver_block_erase = DriverCallback::eraseBlock;
    norFlash.lx_nor_flash_driver_block_erased_verify = DriverCallback::verifyErasedBlock;
    norFlash.lx_nor_flash_driver_system_error = DriverCallback::systemError;

    return std::to_underlying(norFlash.initialiseCallback());
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::DriverCallback::read(ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress,
                                               ThreadX::Ulong *destination, ThreadX::Ulong words)
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.readCallback(flashAddress, destination, words));
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::DriverCallback::write(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong *flashAddress, ThreadX::Ulong *source,
    ThreadX::Ulong words)
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.writeCallback(flashAddress, source, words));
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::DriverCallback::eraseBlock(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block, ThreadX::Ulong eraseCount)
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.eraseBlockCallback(block, eraseCount));
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::DriverCallback::verifyErasedBlock(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Ulong block)
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.verifyErasedBlockCallback(block));
}

template <ThreadX::Ulong CacheSize>
auto NorFlash<CacheSize>::DriverCallback::systemError(
    ThreadX::Native::LX_NOR_FLASH *norFlashPtr, ThreadX::Uint errorCode)
{
    auto &norFlash{static_cast<NorFlash &>(*norFlashPtr)};
    return std::to_underlying(norFlash.systemErrorCallback(errorCode));
}
} // namespace LevelX