#include "vosfs/raft/state_machine.hpp"
#include <kosio/common/debug.hpp>
#include <kosio/runtime/runtime.hpp>

void vosfs::raft::StateMachine::apply(const LogEntry& entry) {
    EntryCommand command;
    if (!command.ParseFromString(entry.command())) {
        LOG_FATAL("Failed to parse command at entry {}", entry.index());
    }

    switch (command.cmd_case()) {
        case EntryCommand::kCreateFileCommitRequest: {

            break;
        }
        default: {
            LOG_FATAL("Failed to find command at entry {}", entry.index());
            break;
        }
    }
}
