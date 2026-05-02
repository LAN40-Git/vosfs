file(GLOB SRCS
        "../../include/vosfs/ui/*.hpp"
        "../../api/authpb/*.cc"
        "../../api/raftpb/*.cc"
        "*.cpp"
)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(vosfs_client
        main.cpp
)

qt_add_qml_module(vosfs_client
        URI main
        QML_FILES
        Main.qml
        LoginPage.qml
        RegisterPage.qml
        SOURCES ${SRCS}
        RESOURCES res.qrc
)

target_link_libraries(vosfs_client
        PRIVATE
        ${DEPEND_LIBS}
)