file(GLOB RAFT_SRCS
        "../../api/raftpb/*.cc"
        "raft/*.cpp"
        "raft/detail/*.cpp"
)

set(VOSFS_SRCS
        ${RAFT_SRCS}
        PARENT_SCOPE
)