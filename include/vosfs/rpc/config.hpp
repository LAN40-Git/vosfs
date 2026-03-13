#pragma once
#include <cstddef>

namespace vosfs::rpc::detail {
constexpr std::size_t MAX_RPC_MESSAGE_SIZE = 4 * 1024 * 1024; // 4MB
constexpr uint16_t RPC_PORT = 8888;
constexpr uint16_t CLIENT_PORT = 8889;
} // namespace vosfs::rpc::detail