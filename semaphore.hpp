#pragma once

#include "tickTimer.hpp"
#include "txCommon.hpp"
#include <functional>

namespace ThreadX
{
class CountingSemaphoreBase : protected Native::TX_SEMAPHORE
{
  protected:
    explicit CountingSemaphoreBase(const Ulong ceiling);
    ~CountingSemaphoreBase();

    Error release(Ulong count);

  private:
    Ulong m_ceiling;
};

template <Ulong Ceiling = std::numeric_limits<Ulong>::max()> class CountingSemaphore : CountingSemaphoreBase
{
  public:
    using NotifyCallback = std::function<void(CountingSemaphore &)>;

    // none copyable or movable
    CountingSemaphore(const CountingSemaphore &) = delete;
    CountingSemaphore &operator=(const CountingSemaphore &) = delete;

    constexpr auto max() const;

    explicit CountingSemaphore(
        const std::string_view name, const Ulong initialCount = 0, const NotifyCallback &releaseNotifyCallback = {});

    auto acquire();

    // must be used for calls from initialization, timers, and ISRs
    auto tryAcquire();

    template <class Clock, typename Duration>
    auto tryAcquireUntil(const std::chrono::time_point<Clock, Duration> &time);

    /// retrieves an instance (a single count) from the specified counting semaphore.
    /// As a result, the specified semaphore's count is decreased by one.
    /// \param duration
    auto tryAcquireFor(const auto &duration);

    ///  puts an instance into the specified counting semaphore, which in reality increments the counting semaphore by
    ///  one. If the counting semaphore's current value is greater than or equal to the specified ceiling, the instance
    ///  will not be put and a TX_CEILING_EXCEEDED error will be returned.
    /// \param count
    auto release(const Ulong count = 1);

    /// places the highest priority thread suspended for an instance of the semaphore at the front of the suspension
    /// list. All other threads remain in the same FIFO order they were suspended in.
    auto prioritise();

    auto name() const;

    auto count() const;

  private:
    static auto releaseNotifyCallback(auto notifySemaphorePtr);

    const NotifyCallback m_releaseNotifyCallback;
};

template <Ulong Ceiling> constexpr auto CountingSemaphore<Ceiling>::max() const
{
    return Ceiling;
}

/// Constructor
/// \param initialCount
/// \param releaseNotifyCallback The Notifycallback is not allowed to call any ThreadX API with a suspension option.
template <Ulong Ceiling>
CountingSemaphore<Ceiling>::CountingSemaphore(
    const std::string_view name, const Ulong initialCount, const NotifyCallback &releaseNotifyCallback)
    : CountingSemaphoreBase{Ceiling}, m_releaseNotifyCallback{releaseNotifyCallback}
{
    assert(initialCount <= Ceiling);

    using namespace Native;
    [[maybe_unused]] Error error{tx_semaphore_create(this, const_cast<char *>(name.data()), initialCount)};
    assert(error == Error::success);

    if (releaseNotifyCallback)
    {
        error = Error{tx_semaphore_put_notify(this, CountingSemaphore::releaseNotifyCallback)};
        assert(error == Error::success);
    }
}

template <Ulong Ceiling> auto CountingSemaphore<Ceiling>::acquire()
{
    return tryAcquireFor(TickTimer::waitForever);
}

template <Ulong Ceiling> auto CountingSemaphore<Ceiling>::tryAcquire()
{
    return tryAcquireFor(TickTimer::noWait);
}

template <Ulong Ceiling>
template <class Clock, typename Duration>
auto CountingSemaphore<Ceiling>::tryAcquireUntil(const std::chrono::time_point<Clock, Duration> &time)
{
    return tryAcquireFor(time - Clock::now());
}

template <Ulong Ceiling> auto CountingSemaphore<Ceiling>::tryAcquireFor(const auto &duration)
{
    return Error{tx_semaphore_get(this, TickTimer::ticks(duration))};
}

template <Ulong Ceiling> auto CountingSemaphore<Ceiling>::release(const Ulong count)
{
    return CountingSemaphoreBase::release(count);
}

template <Ulong Ceiling> auto CountingSemaphore<Ceiling>::prioritise()
{
    return Error{tx_semaphore_prioritize(this)};
}

template <Ulong Ceiling> auto CountingSemaphore<Ceiling>::name() const
{
    return std::string_view{tx_semaphore_name};
}

template <Ulong Ceiling> auto CountingSemaphore<Ceiling>::count() const
{
    return tx_semaphore_count;
}

template <Ulong Ceiling> auto CountingSemaphore<Ceiling>::releaseNotifyCallback(auto notifySemaphorePtr)
{
    auto &semaphore{static_cast<CountingSemaphore &>(*notifySemaphorePtr)};
    semaphore.m_releaseNotifyCallback(semaphore);
}

using BinarySemaphore = CountingSemaphore<1>;
} // namespace ThreadX
