#pragma once
#include <functional>
#include <format>
#include <kosio/async/coroutine/task.hpp>
#include "vosfs/rpc/result.hpp"

namespace vosfs::rpc {
using RpcRequestHandler = std::function<kosio::async::Task<RpcResult>(std::string_view, std::span<char>)>;

enum class ServiceType : uint8_t {
    kMinService = 0,
    kRaft,
    kAuth,
    kMaxService
};

enum class MethodType : uint8_t {
    kMinMethod = 0,
    kRaftRequestVote,
    kRaftAppendEntries,
    kRaftInstallSnapshot,
    kAuthPutUser,
    kAuthGetUser,
    kAuthUpdateUserName,
    kAuthUpdateUserPassword,
    kAuthUpdateUserRole,
    kAuthDeleteUser,
    kMaxMethod
};

class RpcType {
public:
    [[nodiscard]]
    static auto to_string(ServiceType service_type) -> std::string_view {
        switch (service_type) {
            case ServiceType::kRaft:
                return "Raft";
            case ServiceType::kAuth:
                return "Auth";
            default:
                return "unknown service type";
        }
    }

    [[nodiscard]]
    static auto to_string(MethodType method_type) -> std::string_view {
        switch (method_type) {
            case MethodType::kRaftRequestVote:
                return "RaftRequestVote";
            case MethodType::kRaftAppendEntries:
                return "RaftAppendEntries";
            case MethodType::kRaftInstallSnapshot:
                return "RaftInstallSnapshot";
            case MethodType::kAuthPutUser:
                return "AuthPutUser";
            case MethodType::kAuthGetUser:
                return "AuthGetUser";
            case MethodType::kAuthUpdateUserName:
                return "AuthUpdateUserName";
            case MethodType::kAuthUpdateUserPassword:
                return "AuthUpdateUserPassword";
            case MethodType::kAuthUpdateUserRole:
                return "AuthUpdateUserRole";
            case MethodType::kAuthDeleteUser:
                return "AuthDeleteUser";
            default:
                return "unknown method type";
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