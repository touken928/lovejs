cmake_minimum_required(VERSION 3.16)
project(lovejs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制静态链接
set(BUILD_SHARED_LIBS OFF)

# 静态链接 C++ 标准库和运行时
if(MSVC)
    # MSVC: 静态链接运行时库
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(NOT APPLE)
    # GCC/Clang/MinGW (非 macOS): 静态链接 libgcc 和 libstdc++
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 平台检测
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
endif()

# 第三方库
include(cmake/quickjs.cmake)
include(cmake/sdl3.cmake)

# 源文件
set(LOVEJS_SOURCES
    src/main.cpp
)

# 可执行文件
add_executable(lovejs ${LOVEJS_SOURCES})

target_include_directories(lovejs PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/sdl3_compat_include
)

# 链接第三方库
target_link_libraries(lovejs PRIVATE
    quickjs
    SDL3::SDL3-static
)

# 性能优化和二进制压缩
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # MSVC 优化
        target_compile_options(lovejs PRIVATE /O2 /Ob2 /DNDEBUG)
        target_link_options(lovejs PRIVATE /OPT:REF /OPT:ICF)
    else()
        # GCC/Clang 优化
        target_compile_options(lovejs PRIVATE -O3 -DNDEBUG -ffunction-sections -fdata-sections)
        if(APPLE)
            target_link_options(lovejs PRIVATE -Wl,-dead_strip)
        else()
            target_link_options(lovejs PRIVATE -Wl,--gc-sections -s)
        endif()
    endif()
endif()

# 平台特定设置
if(WIN32)
    target_compile_definitions(lovejs PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(APPLE)
    target_link_libraries(lovejs PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework Metal"
        "-framework QuartzCore"
    )
endif()

