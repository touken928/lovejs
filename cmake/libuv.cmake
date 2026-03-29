# third_party/libuv — 静态库 `uv_a`，封装为 `qianjs::libuv`。

if(TARGET qianjs::libuv)
    return()
endif()

get_filename_component(_qianjs_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_root "${_qianjs_root}/third_party/libuv")
if(NOT EXISTS "${_root}/CMakeLists.txt")
    message(FATAL_ERROR "libuv missing at ${_root}.\n  git submodule update --init third_party/libuv")
endif()

set(LIBUV_BUILD_SHARED OFF CACHE BOOL "Build libuv shared library" FORCE)
set(LIBUV_BUILD_TESTS OFF CACHE BOOL "Build libuv unit tests" FORCE)

add_subdirectory("${_root}" "${CMAKE_BINARY_DIR}/third_party/libuv" EXCLUDE_FROM_ALL)

add_library(qianjs::libuv ALIAS uv_a)
