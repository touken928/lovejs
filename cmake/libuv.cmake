# libuv — cross-platform async I/O (https://github.com/libuv/libuv)
# Submodule: third_party/libuv

set(_qianjs_libuv_root "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libuv")
if(NOT EXISTS "${_qianjs_libuv_root}/CMakeLists.txt")
    message(FATAL_ERROR
        "libuv missing at ${_qianjs_libuv_root}.\n"
        "  git submodule update --init third_party/libuv")
endif()

set(LIBUV_BUILD_SHARED OFF CACHE BOOL "Build libuv shared library" FORCE)
set(LIBUV_BUILD_TESTS OFF CACHE BOOL "Build libuv unit tests" FORCE)

add_subdirectory("${_qianjs_libuv_root}" "${CMAKE_BINARY_DIR}/third_party/libuv" EXCLUDE_FROM_ALL)

add_library(qianjs::libuv ALIAS uv_a)
