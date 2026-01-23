file(GLOB RPC_SRCS "rpc/*.cpp" "rpc/*.cc")
file(GLOB PROTO_SRCS "rpc/pb/*.cc")

set(VOSFS_SRCS
        ${RPC_SRCS}
        ${PROTO_SRCS}
        PARENT_SCOPE
)