# third_party/qjs — QuickJS C++ 封装（上游定义 `qjs::qjs` 等，此处不新增 target 名）。

set(_root "${CMAKE_CURRENT_SOURCE_DIR}/third_party/qjs")
if(NOT EXISTS "${_root}/CMakeLists.txt")
    message(FATAL_ERROR "qjs missing at ${_root}.\n  git submodule update --init third_party/qjs")
endif()

add_subdirectory("${_root}")
