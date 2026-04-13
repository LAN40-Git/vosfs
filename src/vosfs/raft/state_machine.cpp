#include "vosfs/raft/state_machine.hpp"
#include <kosio/common/debug.hpp>
#include <kosio/runtime/runtime.hpp>

void vosfs::raft::StateMachine::apply(std::span<const LogEntry> entries) {
    for (const auto& entry : entries) {
        auto index = entry.index();

        ClientCommand command;
        if (!command.ParseFromString(entry.command())) {
            LOG_FATAL("Failed to parse command at entry {}", index);
            continue;
        }

        switch (command.cmd_case()) {
            case ClientCommand::kUploadFile: {

                break;
            }
            default: {
                LOG_FATAL("Failed to find command at entry {}", index);
                break;
            }
        }
    }
}
