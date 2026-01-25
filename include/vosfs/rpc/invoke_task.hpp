#pragma once
#include "vosfs/rpc/util.hpp"

namespace vosfs::rpc::detail {
using Invoke = std::function<kosio::async::Task<Result<std::size_t>>(std::string_view, std::span<char>, uint64_t, uint64_t)>;
struct InvokeTask {
public:
    InvokeTask() = default;
    explicit InvokeTask(Invoke&& invoke)
        : invoke_(std::move(invoke)) {}

    // Delete copy
    InvokeTask(const InvokeTask&) = delete;
    auto operator=(const InvokeTask&) = delete;

    explicit InvokeTask(uint64_t request_id, Invoke&& invoke)
    : request_id_(request_id)
    , invoke_(std::move(invoke)) {}

    explicit InvokeTask(uint64_t request_id, Invoke&& invoke, std::string&& req_payload)
    : request_id_(request_id)
    , invoke_(std::move(invoke))
    , req_payload_(std::move(req_payload)) {}

public:
    uint64_t    request_id_{0};
    Invoke      invoke_;
    std::string req_payload_{};
};
} // namespace vosfs::rpc::detail