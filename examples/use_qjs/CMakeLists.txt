# Minimal consumer: add_subdirectory this repo and link only the qjs library (no qianjs CLI).
cmake_minimum_required(VERSION 3.16)
project(use_qjs_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QIANJS_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(QIANJS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(QJS_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. qianjs_build)

add_executable(use_qjs_example main.cc)
target_link_libraries(use_qjs_example PRIVATE qjs::qjs)
