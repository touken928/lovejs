# BS::thread_pool — header-only (https://github.com/bshoshany/thread-pool).
# Submodule: third_party/thread-pool

set(_qianjs_thread_pool_root "${CMAKE_CURRENT_SOURCE_DIR}/third_party/thread-pool")
if(NOT EXISTS "${_qianjs_thread_pool_root}/include/BS_thread_pool.hpp")
    message(FATAL_ERROR
        "thread-pool (BS_thread_pool.hpp) missing at ${_qianjs_thread_pool_root}.\n"
        "  git submodule update --init third_party/thread-pool")
endif()

add_library(qianjs_thread_pool INTERFACE)
target_include_directories(qianjs_thread_pool INTERFACE "${_qianjs_thread_pool_root}/include")

add_library(qianjs::thread_pool ALIAS qianjs_thread_pool)
