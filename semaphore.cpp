#include "semaphore.hpp"
#include <cassert>

namespace ThreadX
{
CountingSemaphoreBase::CountingSemaphoreBase(const Ulong ceiling) : Native::TX_SEMAPHORE{}, m_ceiling{ceiling}
{
}

CountingSemaphoreBase::~CountingSemaphoreBase()
{
    [[maybe_unused]] Error error{tx_semaphore_delete(this)};
    assert(error == Error::success);
}

Error CountingSemaphoreBase::release(Ulong count)
{
    while (count > 0)
    {
        if (Error error{tx_semaphore_ceiling_put(this, m_ceiling)}; error != Error::success)
        {
            return error;
        }

        --count;
    }

    return Error::success;
}
} // namespace ThreadX
