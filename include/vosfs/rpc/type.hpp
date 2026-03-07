#pragma once
#include <cstdint>
#include <string_view>

namespace vosfs::rpc {
enum class ServiceType : uint8_t {
    kConnection = 0,
    kRaft,
    kMath, // for test
};

enum class MethodType : uint8_t {
    kConnectionShutdown = 0,
    kRaftRequestVote,
    kRaftAppendEntries,
    kRaftInstallSnapshot,
    kMathAdd,
    kMathSub,
    kMathMul,
    kMathDiv,
};

class RpcType {
public:
    [[nodiscard]]
    static auto to_string(ServiceType service_type) -> std::string_view {
        switch (service_type) {
            case ServiceType::kConnection:
                return "Connection";
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
            case MethodType::kConnectionShutdown:
                return "ConnectionShutdown";
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
            case MethodType::kMathMul:
                return "MathMul";
            case MethodType::kMathDiv:
                return "MathDiv";
            default:
                return "Unknown method type";
        }
    }
};
} // namespace vosfs::rpc