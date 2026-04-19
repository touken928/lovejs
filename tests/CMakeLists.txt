# Mirrors src/: runtime / cli / native — add *_test.cc files in QIANJS_TEST_SOURCES.

set(QIANJS_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/embed_test.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/runtime_context_test.cc
)

if(QIANJS_BUILD_CLI)
    list(APPEND QIANJS_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cli/cli_test.cc)
endif()

add_executable(qianjs_tests ${QIANJS_TEST_SOURCES})

target_include_directories(qianjs_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(qianjs_tests PRIVATE
    qianjs::gtest_main
)

if(QIANJS_BUILD_CLI)
    target_link_libraries(qianjs_tests PRIVATE qianjs_impl)
endif()

add_test(NAME qianjs_tests COMMAND qianjs_tests)
