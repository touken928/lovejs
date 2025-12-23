# SDL2 构建配置
# 使用 git submodule 拉取到 third_party/SDL

set(SDL2_DIR ${CMAKE_SOURCE_DIR}/third_party/SDL)

if(NOT EXISTS ${SDL2_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "SDL2 not found. Please run: git submodule update --init --recursive")
endif()

# SDL2 构建选项 - 静态链接
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

add_subdirectory(${SDL2_DIR} ${CMAKE_BINARY_DIR}/third_party/SDL EXCLUDE_FROM_ALL)

# 创建 SDL2/ 目录结构的符号链接，支持 #include <SDL2/SDL.h> 格式
set(SDL2_COMPAT_INCLUDE_DIR ${CMAKE_BINARY_DIR}/sdl2_compat_include/SDL2)
file(MAKE_DIRECTORY ${SDL2_COMPAT_INCLUDE_DIR})

file(GLOB SDL2_HEADERS ${SDL2_DIR}/include/*.h)
foreach(header ${SDL2_HEADERS})
    get_filename_component(header_name ${header} NAME)
    file(CREATE_LINK ${header} ${SDL2_COMPAT_INCLUDE_DIR}/${header_name} SYMBOLIC)
endforeach()
