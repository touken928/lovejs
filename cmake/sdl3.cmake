# SDL3 构建配置
# 使用 git submodule 拉取到 third_party/SDL

set(SDL3_DIR ${CMAKE_SOURCE_DIR}/third_party/SDL)

if(NOT EXISTS ${SDL3_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "SDL3 not found. Please run: git submodule update --init --recursive")
endif()

# SDL3 构建选项 - 静态链接
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

# 禁用不需要的功能以减少编译时间
set(SDL_AUDIO ON CACHE BOOL "" FORCE)
set(SDL_VIDEO ON CACHE BOOL "" FORCE)
set(SDL_RENDER ON CACHE BOOL "" FORCE)
set(SDL_EVENTS ON CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)

add_subdirectory(${SDL3_DIR} ${CMAKE_BINARY_DIR}/third_party/SDL EXCLUDE_FROM_ALL)

# 创建 SDL3/ 目录结构的符号链接，支持 #include <SDL3/SDL.h> 格式
set(SDL3_COMPAT_INCLUDE_DIR ${CMAKE_BINARY_DIR}/sdl3_compat_include/SDL3)
file(MAKE_DIRECTORY ${SDL3_COMPAT_INCLUDE_DIR})

file(GLOB SDL3_HEADERS ${SDL3_DIR}/include/SDL3/*.h)
foreach(header ${SDL3_HEADERS})
    get_filename_component(header_name ${header} NAME)
    file(CREATE_LINK ${header} ${SDL3_COMPAT_INCLUDE_DIR}/${header_name} SYMBOLIC)
endforeach()