# SDL2_image 构建配置
# 使用 git submodule 拉取到 third_party/SDL_image

set(SDL2_IMAGE_DIR ${CMAKE_SOURCE_DIR}/third_party/SDL_image)

if(NOT EXISTS ${SDL2_IMAGE_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "SDL2_image not found. Please run: git submodule update --init --recursive")
endif()

# SDL2_image 构建选项 - 静态链接
set(SDL2IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# 图片格式支持 - 只启用常用格式
set(SDL2IMAGE_PNG ON CACHE BOOL "" FORCE)
set(SDL2IMAGE_JPG ON CACHE BOOL "" FORCE)
set(SDL2IMAGE_BMP ON CACHE BOOL "" FORCE)

# 禁用不需要的格式
set(SDL2IMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_GIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_LBM OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_PCX OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_PNM OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_QOI OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_SVG OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TGA OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_XCF OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_XPM OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_XV OFF CACHE BOOL "" FORCE)

# 使用 vendored 库（内置的 libpng/libjpeg）
set(SDL2IMAGE_VENDORED ON CACHE BOOL "" FORCE)

# 设置 SDL2 路径，让 SDL_image 找到它
set(SDL2_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/SDL/include)
set(SDL2_LIBRARY SDL2-static)

add_subdirectory(${SDL2_IMAGE_DIR} ${CMAKE_BINARY_DIR}/third_party/SDL_image EXCLUDE_FROM_ALL)

# 创建 SDL_image.h 符号链接到 SDL2/ 目录
file(GLOB SDL2_IMAGE_HEADERS ${SDL2_IMAGE_DIR}/include/*.h)
foreach(header ${SDL2_IMAGE_HEADERS})
    get_filename_component(header_name ${header} NAME)
    file(CREATE_LINK ${header} ${SDL2_COMPAT_INCLUDE_DIR}/${header_name} SYMBOLIC)
endforeach()
