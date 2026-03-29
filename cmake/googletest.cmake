# third_party/googletest — 须在 `qjs` 之前 include，以便 `QJS_BUILD_TESTS` 复用同一 GTest。

set(_root "${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest")
if(NOT EXISTS "${_root}/CMakeLists.txt")
    message(FATAL_ERROR "GoogleTest missing at ${_root}.\n  git submodule update --init third_party/googletest")
endif()

set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

add_subdirectory("${_root}" EXCLUDE_FROM_ALL)

add_library(qianjs_gtest_main INTERFACE)
target_link_libraries(qianjs_gtest_main INTERFACE GTest::gtest_main)
add_library(qianjs::gtest_main ALIAS qianjs_gtest_main)
