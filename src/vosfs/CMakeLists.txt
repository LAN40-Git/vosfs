file(GLOB CLIENT_SRCS "client/*.cpp" "client/*.cc")
file(GLOB PROTO_SRCS "rpc/pb/*.cpp" "rpc/pb/*.cc")
file(GLOB RPC_SRCS "rpc/*.cpp" "rpc/*.cc")

set(VOSFS_SRCS
        ${CLIENT_SRCS}
        ${PROTO_SRCS}
        ${RPC_SRCS}
        PARENT_SCOPE
)