#pragma once

#include "fxCommon.hpp"
#include <cassert>
#include <cstring>
#include <functional>
#include <span>
#include <string_view>
#include <utility>

namespace FileX
{
template <MediaSectorSize N> class Media;

enum class OpenOption : ThreadX::Uint
{
    read = FX_OPEN_FOR_READ,
    write = FX_OPEN_FOR_WRITE,
    fastRead = FX_OPEN_FOR_READ_FAST,
};

enum class SeekFrom : ThreadX::Uint
{
    begin = FX_SEEK_BEGIN,
    end = FX_SEEK_END,
    forward = FX_SEEK_FORWARD,
    back = FX_SEEK_BACK
};

enum class AllocateOption
{
    strict,
    bestEffort
};

enum class TruncateOption
{
    noRelease,
    release
};

class File : ThreadX::Native::FX_FILE
{
  public:
    using UlongPair = std::pair<Error, ThreadX::Ulong>;
    using Ulong64Pair = std::pair<Error, ThreadX::Ulong64>;
    using NotifyCallback = std::function<void(File &)>;

    template <MediaSectorSize N> explicit File(const std::string_view fileName, Media<N> &media, const OpenOption option = OpenOption::read, NotifyCallback writeNotifyCallback = {});
    ~File();
    Ulong64Pair allocate(ThreadX::Ulong64 size, AllocateOption option = AllocateOption::strict);
    Error truncate(ThreadX::Ulong64 newSize, TruncateOption option = TruncateOption::noRelease);
    Error seek(const ThreadX::Ulong64 offset);
    Error relativeSeek(const ThreadX::Ulong64 offset, const SeekFrom from = SeekFrom::forward);
    Error write(const std::span<std::byte> data);
    Error write(const std::string_view str);
    UlongPair read(std::span<std::byte> buffer);
    UlongPair read(std::span<std::byte> buffer, const ThreadX::Ulong size);

  private:
    static void writeNotifyCallback(ThreadX::Native::FX_FILE *notifyFilePtr);

    const NotifyCallback m_writeNotifyCallback;
};

template <MediaSectorSize N> File::File(const std::string_view fileName, Media<N> &media, const OpenOption option, NotifyCallback writeNotifyCallback) : ThreadX::Native::FX_FILE{}, m_writeNotifyCallback{writeNotifyCallback}
{
    using namespace ThreadX::Native;
    [[maybe_unused]] Error error{fx_file_open(std::addressof(media), this, const_cast<char *>(fileName.data()), std::to_underlying(option))};
    assert(error == Error::success);

    if (m_writeNotifyCallback)
    {
        error = Error{fx_file_write_notify_set(this, File::writeNotifyCallback)};
        assert(error == Error::success);
    }
}
} // namespace FileX
