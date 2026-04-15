#include "vosfs/raft/state_machine.hpp"
#include <kosio/common/debug.hpp>
#include <kosio/runtime/runtime.hpp>

void vosfs::raft::StateMachine::apply(const LogEntry& entry) {
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

void vosfs::raft::StateMachine::apply_snapshot(const Snapshot& snapshot) {
    inodes_.clear();
    auto& inodes = snapshot.inodes();
    for (const auto& inode : inodes) {
        inodes_.emplace(inode.ino(), inode);
    }
}
