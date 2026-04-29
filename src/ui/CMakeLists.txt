file(GLOB SRCS
        "../../include/vosfs/ui/*.hpp"
        "../../api/authpb/*.cc"
        "*.cpp"
)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(vosfs_ui
        main.cpp
)

qt_add_qml_module(vosfs_ui
        URI main
        QML_FILES
        Main.qml
        SOURCES ${SRCS}
        RESOURCES res.qrc
)

target_link_libraries(vosfs_ui
        PRIVATE
        ${DEPEND_LIBS}
)