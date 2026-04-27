#pragma once
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace vosfs::util {
// -1: other error, 0: not found, 1: found
static auto file_exists(const char* path) -> int {
    struct statx sx;
    auto ret = statx(AT_FDCWD, path, 0, 0, &sx);

    if (ret == 0) {
        return 1;
    }
    if (errno == ENOENT) {
        return 0;
    }

    return -1;
}
} // namespace vosfs::util
