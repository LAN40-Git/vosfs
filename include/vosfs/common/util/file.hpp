#pragma once
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace vosfs::util {
static auto get_file_size(const char* path) -> int {
    struct statx sx {};
    int ret = statx(AT_FDCWD, path, AT_SYMLINK_NOFOLLOW, STATX_SIZE, &sx);
    if (ret == -1) {
        if (errno == ENOENT) {
            return 0;
        } else {
            return -1;
        }
    }

    return sx.stx_size;
}
} // namespace vosfs::util
