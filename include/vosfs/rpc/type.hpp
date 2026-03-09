#pragma once
#include <cstdint>
#include <format>
#include <string_view>

namespace vosfs::rpc {
enum class ServiceType : uint8_t {
    kMinService = 0,
    kAuth,
    kSession,
    kRaft,
    kMath, // for test
    kMaxService
};

enum class MethodType : uint8_t {
    kMinMethod = 0,
    kAuthSignup,
    kAuthSignin,
    kAuthSignout,
    kSessionClose,
    kRaftRequestVote,
    kRaftAppendEntries,
    kRaftInstallSnapshot,
    kMathAdd,
    kMathSub,
    kMathMul,
    kMathDiv,
    kMaxMethod
};

class RpcType {
public:
    [[nodiscard]]
    static auto to_string(ServiceType service_type) -> std::string_view {
        switch (service_type) {
            case ServiceType::kAuth:
                return "Auth";
            case ServiceType::kSession:
                return "Session";
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
            case MethodType::kAuthSignup:
                return "AuthSignup";
            case MethodType::kAuthSignin:
                return "AuthSignin";
            case MethodType::kAuthSignout:
                return "AuthSignout";
            case MethodType::kSessionClose:
                return "SessionClose";
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