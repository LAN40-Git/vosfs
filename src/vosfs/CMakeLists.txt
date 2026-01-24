file(GLOB PROTO_SRCS "rpc/pb/*.cpp" "rpc/pb/*.cc")
file(GLOB RPC_SRCS "rpc/*.cpp" "rpc/*.cc")

set(VOSFS_SRCS
        ${PROTO_SRCS}
        ${RPC_SRCS}
        PARENT_SCOPE
)