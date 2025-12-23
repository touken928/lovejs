cmake_minimum_required(VERSION 3.16)
project(lovejs LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
    ${CMAKE_BINARY_DIR}/sdl3_compat_include  # SDL3/SDL.h 格式
)

# 链接第三方库
target_link_libraries(lovejs PRIVATE
    quickjs
    SDL3::SDL3-static
)

# Windows 特定设置
if(WIN32)
    target_compile_definitions(lovejs PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Windows 下隐藏控制台窗口（Release 模式）
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(lovejs PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

# macOS 特定设置
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
