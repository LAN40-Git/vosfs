file(GLOB API_SRCS
        "api/mathpb/*.cc"
        "api/serverpb/*.cc"
)

file(GLOB COMMON_SRCS
        "common/*.cpp"
)

file(GLOB AUTH_SRCS
        "auth/*.cpp"
        "auth/internal/*.cpp"
)

file(GLOB RAFT_SRCS
        "raft/*.cpp"
        "raft/internal/*.cpp"
)

file(GLOB RPC_SRCS
        "rpc/*.cpp"
        "rpc/internal/*.cpp"
)

set(VOSFS_SRCS
        ${API_SRCS}
        ${COMMON_SRCS}
        ${AUTH_SRCS}
        ${RAFT_SRCS}
        ${RPC_SRCS}
        PARENT_SCOPE
)