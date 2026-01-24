#pragma once
#include <cstdint>
#include <string_view>

namespace vosfs::rpc {
namespace detail {
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
} // namespace detail

class RpcType {
public:
    [[nodiscard]]
    static auto to_string(detail::ServiceType service_type) -> std::string_view {
        switch (service_type) {
            case detail::ServiceType::kConnection:
                return "Connection";
            case detail::ServiceType::kRaft:
                return "Raft";
            case detail::ServiceType::kMath:
                return "Math";
            default: return "Unknown service type";
        }
    }

    [[nodiscard]]
    static auto to_string(detail::MethodType method_type) -> std::string_view {
        switch (method_type) {
            case detail::MethodType::kConnectionShutdown:
                return "ConnectionShutdown";
            case detail::MethodType::kRaftRequestVote:
                return "RaftRequestVote";
            case detail::MethodType::kRaftAppendEntries:
                return "RaftAppendEntries";
            case detail::MethodType::kRaftInstallSnapshot:
                return "RaftInstallSnapshot";
            case detail::MethodType::kMathAdd:
                return "MathAdd";
            case detail::MethodType::kMathSub:
                return "MathSub";
            case detail::MethodType::kMathMul:
                return "MathMul";
            case detail::MethodType::kMathDiv:
                return "MathDiv";
            default:
                return "Unknown method type";
        }
    }
};
} // namespace vosfs::rpc