#include "memoryPool.hpp"

namespace ThreadX
{
BytePoolBase::BytePoolBase() : Native::TX_BYTE_POOL{}
{
}

BytePoolBase::~BytePoolBase()
{
    [[maybe_unused]] Error error{tx_byte_pool_delete(this)};
    assert(error == Error::success);
}

Error BytePoolBase::prioritise()
{
    return Error{tx_byte_pool_prioritize(this)};
}

std::string_view BytePoolBase::name() const
{
    return std::string_view(tx_byte_pool_name);
}

void BytePoolBase::create(const std::string_view name, void *const poolStartPtr, const Ulong Size)
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_byte_pool_create(this, const_cast<char *>(name.data()), poolStartPtr, Size)};
    assert(error == Error::success);
}

BlockPoolBase::BlockPoolBase() : Native::TX_BLOCK_POOL{}
{
}

BlockPoolBase::~BlockPoolBase()
{
    [[maybe_unused]] Error error{tx_block_pool_delete(this)};
    assert(error == Error::success);
}

Ulong BlockPoolBase::blockSize() const
{
    return tx_block_pool_block_size;
}

Error BlockPoolBase::prioritise()
{
    return Error{tx_block_pool_prioritize(this)};
}

std::string_view BlockPoolBase::name() const
{
    return std::string_view(tx_block_pool_name);
}

void BlockPoolBase::create(const std::string_view name, const Ulong BlockSize, void *const poolStartPtr, const Ulong Size)
{
    using namespace Native;
    [[maybe_unused]] Error error{tx_block_pool_create(this, const_cast<char *>(name.data()), BlockSize, poolStartPtr, Size)};
    assert(error == Error::success);
}
} // namespace ThreadX
