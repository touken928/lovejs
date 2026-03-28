# qjs C++ wrapper + QuickJS (FetchContent in third_party/qjs/cmake/quickjs.cmake).
# Submodule: third_party/qjs

set(_qianjs_qjs_root "${CMAKE_CURRENT_SOURCE_DIR}/third_party/qjs")
if(NOT EXISTS "${_qianjs_qjs_root}/CMakeLists.txt")
    message(FATAL_ERROR
        "qjs missing at ${_qianjs_qjs_root}.\n"
        "  git submodule update --init third_party/qjs")
endif()

add_subdirectory(${_qianjs_qjs_root})
