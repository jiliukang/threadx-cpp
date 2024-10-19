#include "tickTimer.hpp"

namespace ThreadX
{
TickTimer::TimePoint TickTimer::now()
{
    return TimePoint{Duration{Native::tx_time_get()}};
}

TickTimer::~TickTimer()
{
    [[maybe_unused]] Error error{tx_timer_delete(this)};
    assert(error == Error::success);
}

Error TickTimer::activate()
{
    return Error{tx_timer_activate(this)};
}

Error TickTimer::deactivate()
{
    return Error{tx_timer_deactivate(this)};
}

Error TickTimer::reactivate()
{
    return change(m_timeout);
}

size_t TickTimer::id() const
{
    return m_id;
}

std::string_view TickTimer::name() const
{
    return std::string_view{tx_timer_name};
}

void TickTimer::expirationCallback(const Ulong timerPtr)
{
    auto &timer{*reinterpret_cast<TickTimer *>(timerPtr)};
    timer.m_expirationCallback(timer.m_id);
}
} // namespace ThreadX
