enable_testing()

add_executable(qianjs_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/embed_test.cc
)

target_include_directories(qianjs_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(qianjs_tests PRIVATE
    qianjs::gtest_main
)

include(GoogleTest)
gtest_discover_tests(qianjs_tests)
