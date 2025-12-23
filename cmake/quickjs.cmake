# QuickJS 构建配置
# 使用 git submodule 拉取到 third_party/quickjs

set(QUICKJS_DIR ${CMAKE_SOURCE_DIR}/third_party/quickjs)

if(NOT EXISTS ${QUICKJS_DIR}/quickjs.c)
    message(FATAL_ERROR "QuickJS not found. Please run: git submodule update --init --recursive")
endif()

# QuickJS 源文件
set(QUICKJS_SOURCES
    ${QUICKJS_DIR}/quickjs.c
    ${QUICKJS_DIR}/libregexp.c
    ${QUICKJS_DIR}/libunicode.c
    ${QUICKJS_DIR}/cutils.c
    ${QUICKJS_DIR}/dtoa.c
)

# 在构建目录创建只包含头文件的 include 目录（符号链接）
set(QUICKJS_INCLUDE_DIR ${CMAKE_BINARY_DIR}/quickjs_include)
file(MAKE_DIRECTORY ${QUICKJS_INCLUDE_DIR})

# 需要的头文件列表
set(QUICKJS_HEADERS
    quickjs.h
)

foreach(header ${QUICKJS_HEADERS})
    if(EXISTS ${QUICKJS_DIR}/${header})
        file(CREATE_LINK ${QUICKJS_DIR}/${header} ${QUICKJS_INCLUDE_DIR}/${header} SYMBOLIC)
    endif()
endforeach()

# 创建静态库
add_library(quickjs STATIC ${QUICKJS_SOURCES})

# quickjs 库本身使用原始路径编译
target_include_directories(quickjs PRIVATE ${QUICKJS_DIR})

# 对外暴露只包含头文件的目录
target_include_directories(quickjs PUBLIC ${QUICKJS_INCLUDE_DIR})

# 编译选项
target_compile_definitions(quickjs PRIVATE
    CONFIG_VERSION="2024-01-13"
)

if(WIN32)
    target_compile_definitions(quickjs PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
elseif(UNIX)
    target_compile_definitions(quickjs PRIVATE
        _GNU_SOURCE
    )
    target_link_libraries(quickjs PRIVATE m pthread dl)
endif()

# 禁用一些警告
if(MSVC)
    target_compile_options(quickjs PRIVATE /W3 /wd4244 /wd4267 /wd4996)
else()
    target_compile_options(quickjs PRIVATE -w)
endif()
