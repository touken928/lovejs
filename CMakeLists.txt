cmake_minimum_required(VERSION 3.16)

project(qianjs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)

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

if(QIANJS_BUILD_TESTS)
    include(cmake/googletest.cmake)
    add_subdirectory(tests)
endif()

include(cmake/qjs.cmake)

option(QIANJS_BUILD_CLI "Build the qianjs command-line executable" ON)

if(QIANJS_BUILD_CLI)
    include(${CMAKE_CURRENT_SOURCE_DIR}/src/native/native_modules.cmake)
    include(cmake/libuv.cmake)
    include(cmake/uvw.cmake)

    find_package(Threads REQUIRED)

    add_executable(qianjs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cli/main.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/event_loop/event_loop.cc
    )

    target_include_directories(qianjs PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    )

    target_compile_definitions(qianjs PRIVATE
        "QIANJS_HAVE_LIBUV=$<OR:$<BOOL:${QIANJS_MODULE_FS}>,$<BOOL:${QIANJS_MODULE_NET}>>"
    )

    target_link_libraries(qianjs PRIVATE
        qjs::qjs
        Threads::Threads
    )
    if(QIANJS_MODULE_FS OR QIANJS_MODULE_NET)
        target_link_libraries(qianjs PRIVATE qianjs::libuv qianjs::uvw)
    endif()

    if(NOT APPLE)
        target_link_options(qianjs PRIVATE -static-libgcc -static-libstdc++)
        if(WIN32)
            target_link_options(qianjs PRIVATE -static)
        endif()
    endif()

    # src/native/CMakeLists.txt: appends module sources, runs qianjs_write_native_glue (see src/native/native_modules.cmake).
    add_subdirectory(src/native)

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
