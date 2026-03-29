# Built-in native modules: sources for `qianjs` (options from native_modules.cmake in this directory, included by root).

if(QIANJS_MODULE_FS)
    target_sources(qianjs PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_uv.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_ops.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_stat_js.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_sync.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/fs/fs_module.cc
    )
endif()

if(QIANJS_MODULE_NET)
    target_sources(qianjs PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/net/net_uv.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/net/net_module.cc
    )
endif()

# console: header-only plugin, no extra translation units

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
qianjs_write_native_glue("${CMAKE_BINARY_DIR}/generated")
