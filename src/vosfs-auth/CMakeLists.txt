file(GLOB AUTH_SRCS
        "../../api/authpb/*.cc"
        "*.cpp"
)

set(VOSFS_AUTH_SRCS
        ${AUTH_SRCS}
        PARENT_SCOPE
)