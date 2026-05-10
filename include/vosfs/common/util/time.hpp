#pragma once
#include <ctime>
#include <cstdint>

namespace vosfs::util {
static inline auto current_ms() noexcept -> uint64_t {
    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<uint64_t>(now.tv_sec) * 1000ULL
         + static_cast<uint64_t>(now.tv_nsec) / 1'000'000ULL;
}
} // namespace vosfs::util