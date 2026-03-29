cmake_minimum_required(VERSION 3.16)

project(qianjs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制静态链接
set(BUILD_SHARED_LIBS OFF)

# 静态链接 C++ 标准库和运行时
if(NOT APPLE)
    # GCC/Clang/MinGW: 静态链接 libgcc 和 libstdc++
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    if(WIN32)
        # MinGW: 静态链接 pthread
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 平台检测
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
endif()

option(QIANJS_BUILD_TESTS "Build QianJS tests (third_party/googletest submodule)" ON)

# GoogleTest before qjs so QJS_BUILD_TESTS=ON can reuse GTest::gtest_main (see third_party/qjs/cmake/googletest.cmake).
if(QIANJS_BUILD_TESTS)
    include(cmake/googletest.cmake)
endif()

include(cmake/qjs.cmake)

option(QIANJS_BUILD_CLI "Build the qianjs command-line executable" ON)

if(QIANJS_BUILD_CLI)
    include(cmake/libuv.cmake)
    find_package(Threads REQUIRED)

    set(QIANJS_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cli/main.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/event_loop/event_loop.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/native/fs/fs_uv.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/native/fs/fs_ops.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/native/fs/fs_module.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/native/net/net_uv.cc
    )

    add_executable(qianjs ${QIANJS_SOURCES})

    target_include_directories(qianjs PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/uvw/src
    )

    target_link_libraries(qianjs PRIVATE
        qjs::qjs
        qianjs::libuv
        Threads::Threads
    )

    # 性能优化和二进制压缩
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # GCC/Clang/MinGW 优化
        target_compile_options(qianjs PRIVATE -O3 -DNDEBUG -ffunction-sections -fdata-sections)
        if(APPLE)
            target_link_options(qianjs PRIVATE -Wl,-dead_strip)
        else()
            target_link_options(qianjs PRIVATE -Wl,--gc-sections -s)
        endif()
    endif()

    # 平台特定设置
    if(WIN32)
        if(MSVC)
            message(FATAL_ERROR "MSVC is not supported. Please use MinGW-w64 or Clang on Windows.")
        endif()

        target_compile_definitions(qianjs PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()
