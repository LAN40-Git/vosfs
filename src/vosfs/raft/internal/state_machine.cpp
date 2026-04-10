#include "vosfs/raft/internal/state_machine.hpp"
#include <kosio/common/debug.hpp>
#include <kosio/runtime/runtime.hpp>

void vosfs::raft::detail::StateMachine::apply(std::span<const LogEntry> entries) {
    for (const auto& entry : entries) {
        auto index = entry.index();
        ClientCommand command;
        if (!command.ParseFromString(entry.command())) {
            LOG_WARN("Failed to parse command at entry {}", index);
            auto it = pending_map_.find(index);
            if (it != pending_map_.end()) {
                auto& context = it->second;
                context.result = rpc::make_result(rpc::RpcResult::kMessageParseFailed);
            }
            continue;
        }


    }
}
