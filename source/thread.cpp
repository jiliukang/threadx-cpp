#include "thread.hpp"
#include <utility>

namespace ThreadX::ThisThread
{
ID id()
{
    return ID(Native::tx_thread_identify());
}

void yield()
{
    Native::tx_thread_relinquish();
}
} // namespace ThreadX::ThisThread
