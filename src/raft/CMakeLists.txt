file(GLOB SRCS
        "*.cpp"
        "../../api/authpb/*.cc"
)

set(RAFT_SRCS ${SRCS} CACHE INTERNAL "auth source files")