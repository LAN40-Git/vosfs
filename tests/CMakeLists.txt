file(GLOB TEST_VOSFS_SRCS PRIVATE
        vosfs/*.cpp
)

file(GLOB TEST_VOSFS_AUTH_SRCS PRIVATE
        vosfs-auth/*.cpp
)

foreach(source ${TEST_VOSFS_SRCS})
    get_filename_component(basename ${source} NAME_WE)
    add_executable(${basename} ${source} ${VOSFS_SRCS})
    set_target_properties(${basename} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/vosfs)
    target_link_libraries(${basename} ${DEPEND_LIBS})
    add_test(NAME ${basename} COMMAND ${basename})
endforeach ()

foreach(source ${TEST_VOSFS_AUTH_SRCS})
    get_filename_component(basename ${source} NAME_WE)
    add_executable(${basename} ${source} ${VOSFS_AUTH_SRCS})
    set_target_properties(${basename} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/vosfs-auth)
    target_link_libraries(${basename} ${DEPEND_LIBS})
    add_test(NAME ${basename} COMMAND ${basename})
endforeach ()