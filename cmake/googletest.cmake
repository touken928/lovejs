# GoogleTest submodule (third_party/googletest) + QianJS unit tests (qianjs_tests).
# Used by the root project only — not qjs FetchContent.
#
# Include this before cmake/qjs.cmake (see root CMakeLists.txt) so that when
# QJS_BUILD_TESTS is ON, third_party/qjs/cmake/googletest.cmake sees GTest::gtest_main
# and does not FetchContent a second copy.

set(_qianjs_gtest_root "${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest")
if(NOT EXISTS "${_qianjs_gtest_root}/CMakeLists.txt")
    message(FATAL_ERROR
        "GoogleTest missing at ${_qianjs_gtest_root}.\n"
        "  git submodule update --init third_party/googletest")
endif()

set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

add_subdirectory(${_qianjs_gtest_root} EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# qianjs_tests
# ---------------------------------------------------------------------------

enable_testing()

add_executable(qianjs_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/embed_test.cc
)

target_include_directories(qianjs_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(qianjs_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(qianjs_tests)
