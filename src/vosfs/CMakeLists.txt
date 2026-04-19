file(GLOB API_SRCS
        "api/serverpb/*.cc"
)

file(GLOB COMMON_SRCS
        "common/*.cpp"
        "common/util/*.cpp"
)

file(GLOB RAFT_SRCS
        "raft/*.cpp"
        "raft/internal/*.cpp"
)

set(VOSFS_SRCS
        ${API_SRCS}
        ${COMMON_SRCS}
        ${AUTH_SRCS}
        ${RAFT_SRCS}
        PARENT_SCOPE
)