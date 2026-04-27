file(GLOB SRCS PRIVATE
        "*.cpp"
        "api"
)

foreach(source ${SRCS})
    get_filename_component(basename ${source} NAME_WE)
    add_executable(${basename} ${source} ${RAFT_SRCS})
    target_link_libraries(${basename} ${DEPEND_LIBS})
endforeach ()