#pragma once
#include <cstddef>

namespace vosfs::rpc::detail {
constexpr std::size_t MAX_RPC_MESSAGE_SIZE = 1 * 1024 * 1024; // 4MB
} // namespace vosfs::rpc::detail