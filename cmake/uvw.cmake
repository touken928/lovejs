# third_party/uvw — 头文件封装，上游 `uvw::uvw`（INTERFACE），此处 `ALIAS` 为 `qianjs::uvw`。

if(TARGET qianjs::uvw)
    return()
endif()

get_filename_component(_qianjs_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_root "${_qianjs_root}/third_party/uvw")
if(NOT EXISTS "${_root}/CMakeLists.txt")
    message(FATAL_ERROR "uvw missing at ${_root}.\n  git submodule update --init third_party/uvw")
endif()

set(UVW_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(UVW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

add_subdirectory("${_root}" "${CMAKE_BINARY_DIR}/third_party/uvw" EXCLUDE_FROM_ALL)

if(NOT TARGET uvw::uvw)
    message(FATAL_ERROR "third_party/uvw did not define target uvw::uvw")
endif()

add_library(qianjs_uvw INTERFACE)
target_link_libraries(qianjs_uvw INTERFACE uvw::uvw)
add_library(qianjs::uvw ALIAS qianjs_uvw)
