file(GLOB SRCS
        "*.cpp"
        "../../api/raftpb/*.cc"
)

set(STORAGE_SRCS ${SRCS} CACHE INTERNAL "storage source files")