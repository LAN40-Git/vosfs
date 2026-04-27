#include "vosfs/raft/state_machine.hpp"
#include <kosio/common/debug.hpp>
#include <kosio/runtime/runtime.hpp>

void vosfs::raft::detail::StateMachine::apply_entry(const LogEntry& entry) {
    EntryCommand command;
    if (!command.ParseFromString(entry.command())) {
        LOG_FATAL("failed to parse command at entry {}", entry.index());
        std::abort();
    }

    switch (command.cmd_case()) {

        default: {
            LOG_FATAL("in command at entry {}", entry.index());
            std::abort();
        }
    }
}
