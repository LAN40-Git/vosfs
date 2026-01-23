#pragma once

namespace vosfs::util {
class Noncopyable {
public:
    Noncopyable(const Noncopyable&) = delete;
    auto operator=(const Noncopyable&) -> Noncopyable& = delete;

protected:
    Noncopyable() = default;
    ~Noncopyable() noexcept = default;
};
} // namespace vosfs::util