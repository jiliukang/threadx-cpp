#include "kernel.hpp"

namespace ThreadX::Kernel
{
CriticalSection::CriticalSection()
{
    lock();
}

CriticalSection::~CriticalSection()
{
    unlock();
}

void CriticalSection::lock()
{
    if (not m_locked.test_and_set())
    {
        using namespace Native;
        TX_DISABLE
    }
}

void CriticalSection::unlock()
{
    if (m_locked.test())
    {
        Native::TX_RESTORE;
        m_locked.clear();
    }
}

void start()
{
    Native::tx_kernel_enter();
}

bool inThread()
{
    return Native::tx_thread_identify() ? true : false;
}

bool inIsr()
{
    using namespace Native;
    const Ulong systemState{TX_THREAD_GET_SYSTEM_STATE()};
    return systemState != TX_INITIALIZE_IS_FINISHED and systemState < TX_INITIALIZE_IN_PROGRESS;
}

State state()
{
    using namespace Native;
    return (TX_THREAD_GET_SYSTEM_STATE() < TX_INITIALIZE_IN_PROGRESS) ? State::running : State::uninitialised;
}
}; // namespace ThreadX::Kernel

namespace ThreadX
{
void Native::tx_application_define([[maybe_unused]] void *firstUnusedMemory)
{
    application();
}
} // namespace ThreadX
