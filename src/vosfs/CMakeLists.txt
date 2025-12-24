file(GLOB RPC_SRCS "rpc/*.cpp" "rpc/*.cc")

set(VOSFS_SRCS
        ${RPC_SRCS}
        PARENT_SCOPE
)