cmake_minimum_required(VERSION 3.20)
project(vosfs VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(QT_NO_VULKAN ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(OPTIONS
        ENABLE_KOSIO_LOG
)

set(ENABLE_KOSIO_LOG ON)

foreach (opt IN LISTS OPTIONS)
    option(${opt} "Enable ${opt}" ${${opt}})
endforeach ()

foreach (opt IN LISTS OPTIONS)
    if(${opt})
        add_compile_definitions(${opt})
    endif ()
endforeach ()

message(STATUS "Version: " "${CMAKE_PROJECT_VERSION}")
message(STATUS "C++ standard: " "${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: " "${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build type: " "${CMAKE_BUILD_TYPE}")

find_package(PkgConfig REQUIRED)
# liburing
pkg_check_modules(LIBURING REQUIRED liburing)
# protobuf
find_package(Protobuf REQUIRED)
# rocksdb
find_package(RocksDB REQUIRED)
# tbb
find_package(TBB REQUIRED)
# sqlite
find_package(SQLite3 REQUIRED)
# redis
pkg_check_modules(Hiredis REQUIRED hiredis)
# openssl
find_package(OpenSSL REQUIRED)
# jwt-cpp
find_package(jwt-cpp CONFIG REQUIRED)
# qt
find_package(Qt6 REQUIRED COMPONENTS Quick Core Gui Qml Quick QuickControls2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(DEPEND_LIBS
        Qt6::Quick
        Qt6::Gui
        Qt6::Qml
        Qt6::QuickControls2
        ${LIBURING_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        rocksdb
        tbb
        SQLite::SQLite3
        ${Hiredis_LIBRARIES}
        jwt-cpp::jwt-cpp
)

add_subdirectory(src)
add_subdirectory(tests)