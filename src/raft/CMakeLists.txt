file(GLOB SRCS
        "*.cpp"
        "../../api/raftpb/*.cc"
)

set(RAFT_SRCS ${SRCS} CACHE INTERNAL "raft source files")