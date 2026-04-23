file(GLOB SRCS
        "*.cpp"
        "../../api/authpb/*.cc"
)

set(AUTH_SRCS ${SRCS} CACHE INTERNAL "auth source files")