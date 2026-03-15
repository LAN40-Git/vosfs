file(GLOB API_SRCS
        "api/mathpb/*.cc"
        "api/serverpb/*.cc"
)

file(GLOB COMMON_SRCS
        "common/*.cpp"
)

file(GLOB RAFT_SRCS
        "raft/*.cpp"
)

file(GLOB RPC_SRCS
        "rpc/*.cpp"
)

set(VOSFS_SRCS
        ${API_SRCS}
        ${COMMON_SRCS}
        ${RAFT_SRCS}
        ${RPC_SRCS}
        PARENT_SCOPE
)