#pragma once
#include <functional>
#include <cstdint>
#include <format>
#include <kosio/async/coroutine/task.hpp>
#include "vosfs/rpc/error.hpp"

namespace vosfs::rpc {
using InvokeResult = std::pair<RpcError::ErrorCode, std::size_t>;
using RpcRequestHandler = std::function<kosio::async::Task<InvokeResult>(std::string_view, std::span<char>)>;

enum class ServiceType : uint8_t {
    kMinService = 0,
    kAuth,
    kConn,
    kRaft,
    kMath, // for test
    kMaxService
};

enum class MethodType : uint8_t {
    kMinMethod = 0,
    kAuthSignin,
    kAuthSignout,
    kAuthSignup,
    kConnShutdown,
    kRaftRequestVote,
    kRaftAppendEntries,
    kRaftInstallSnapshot,
    kMathAdd,
    kMathSub,
    kMaxMethod
};

class RpcType {
public:
    [[nodiscard]]
    static auto to_string(ServiceType service_type) -> std::string_view {
        switch (service_type) {
            case ServiceType::kConn:
                return "Conn";
            case ServiceType::kRaft:
                return "Raft";
            case ServiceType::kMath:
                return "Math";
            default: return "Unknown service type";
        }
    }

    [[nodiscard]]
    static auto to_string(MethodType method_type) -> std::string_view {
        switch (method_type) {
            case MethodType::kConnShutdown:
                return "ConnShutdown";
            case MethodType::kRaftRequestVote:
                return "RaftRequestVote";
            case MethodType::kRaftAppendEntries:
                return "RaftAppendEntries";
            case MethodType::kRaftInstallSnapshot:
                return "RaftInstallSnapshot";
            case MethodType::kMathAdd:
                return "MathAdd";
            case MethodType::kMathSub:
                return "MathSub";
            default:
                return "Unknown method type";
        }
    }

    [[nodiscard]]
    static auto is_valid(ServiceType service_type) -> bool {
        return service_type > ServiceType::kMinService &&
               service_type < ServiceType::kMaxService;
    }

    [[nodiscard]]
    static auto is_valid(MethodType method_type) -> bool {
        return method_type > MethodType::kMinMethod &&
               method_type < MethodType::kMaxMethod;
    }
};
} // namespace vosfs::rpc