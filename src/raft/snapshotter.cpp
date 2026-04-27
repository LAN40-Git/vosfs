#include <ranges>
#include "vosfs/raft/snapshotter.hpp"
#include "vosfs/raft/config.hpp"

auto vosfs::raft::detail::Snapshotter::offset_at(uint64_t member_id) -> Task<uint64_t> {
    co_await mutex_.lock();
    co_return offsets_[member_id];
    mutex_.unlock();
}

void vosfs::raft::detail::Snapshotter::reset_offsets() {
    for (auto& offset : offsets_ | std::views::values) {
        offset = 0;
    }
}

auto vosfs::raft::detail::Snapshotter::load_snapshot() -> Result<Snapshot> {
    std::ifstream file(snapshot_path_, std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected{make_error(Error::kFileOpenFailed)};
    }

    std::string buf((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    Snapshot snapshot;
    if (!snapshot.ParseFromString(buf)) {
        return std::unexpected{make_error(Error::kParseFailed)};
    }

    snapshot_size_ = buf.size();
    return snapshot;
}

auto vosfs::raft::detail::Snapshotter::save_snapshot(Snapshot snapshot) -> Task<Result<void>> {
    auto buf = snapshot.SerializeAsString();

    co_await mutex_.lock();
    std::lock_guard lock{mutex_, std::adopt_lock};
    // 写入临时文件
    auto has_file = co_await File::options().truncate(true).create(true).open(tmp_path_);
    if (!has_file) {
        LOG_ERROR("{}", has_file.error());
        co_return std::unexpected{make_error(Error::kFileOpenFailed)};
    }

    auto file = std::move(has_file.value());
    if (auto ret = co_await file.write_all(buf); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kFileWriteFailed)};
    }

    // 原子替换
    if (auto ret = co_await kosio::fs::rename(tmp_path_, snapshot_path_); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kFileRenameFailed)};
    }

    snapshot_size_ = buf.size();
    co_return Result<void>{};
}

auto vosfs::raft::detail::Snapshotter::load_snapshot_data(
    uint64_t member_id, InstallSnapshotRequest& request) -> Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock{mutex_, std::adopt_lock};
    auto& offset = offsets_[member_id];
    auto size = std::min(MAX_SNAPSHOT_CHUNK_SIZE, snapshot_size_ - offset);
    std::string data;
    data.resize(size);

    auto has_file = co_await File::open(snapshot_path_);
    if (!has_file) {
        offset = 0;
        LOG_ERROR("{}", has_file.error());
        co_return std::unexpected{make_error(Error::kFileOpenFailed)};
    }

    auto file = std::move(has_file.value());

    if (auto ret = file.seek(offset, SEEK_SET); ret == -1) {
        offset = 0;
        co_return std::unexpected{make_error(Error::kFileSeekFailed)};
    }

    if (auto ret = co_await file.read_exact(data); !ret) {
        offset = 0;
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kFileReadFailed)};
    }

    request.set_offset(offset);
    offset += size;
    bool done = false;
    if (offset == snapshot_size_) {
        done = true;
        offset = 0;
    }
    request.set_done(done);
    request.set_data(std::move(data));
    co_return Result<void>{};
}

auto vosfs::raft::detail::Snapshotter::save_snapshot_data(std::string data) -> Task<Result<void>> {
    co_await mutex_.lock();
    std::lock_guard lock{mutex_, std::adopt_lock};
    auto has_file = co_await File::open(snapshot_path_);
    if (!has_file) {
        LOG_ERROR("{}", has_file.error());
        co_return std::unexpected{make_error(Error::kFileOpenFailed)};
    }

    auto file = std::move(has_file.value());
    if (auto ret = co_await file.write_all(data); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kFileWriteFailed)};
    }

    // 原子替换
    if (auto ret = co_await kosio::fs::rename(tmp_path_, snapshot_path_); !ret) {
        LOG_ERROR("{}", ret.error());
        co_return std::unexpected{make_error(Error::kFileRenameFailed)};
    }

    co_return Result<void>{};
}
