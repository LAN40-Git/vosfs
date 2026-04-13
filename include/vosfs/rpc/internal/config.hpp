#pragma once
#include <cstddef>

namespace vosfs::rpc::detail {
constexpr std::size_t RECONN_RETRY_TIMES = 3;
constexpr std::size_t MAX_RPC_MESSAGE_SIZE = 4 * 1024 * 1024;
} // namespace vosfs::rpc::detail