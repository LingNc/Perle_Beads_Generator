cmake_minimum_required(VERSION 3.10)
project(PerlerBeadGenerator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找所需的库
find_package(PkgConfig REQUIRED)

# 查找Cairo库
pkg_check_modules(CAIRO REQUIRED cairo)

# 查找FreeType库 (用于字体渲染)
pkg_check_modules(FREETYPE REQUIRED freetype2)

# 查找lodepng库，如果没有则使用内置版本
find_library(LODEPNG_LIB lodepng)
if(NOT LODEPNG_LIB)
    message(STATUS "lodepng library not found, will use built-in version")
    set(USE_BUILTIN_LODEPNG ON)
endif()

# 包含目录
include_directories(${CAIRO_INCLUDE_DIRS})
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(src)

# 编译选项
add_compile_options(${CAIRO_CFLAGS_OTHER})
add_compile_options(${FREETYPE_CFLAGS_OTHER})

# 源文件
set(SOURCES
    src/main.cpp
    src/PerlerGenerator.cpp
    src/ImageProcessor.cpp
    src/ColorPalette.cpp
    src/ColorUtils.cpp
    src/ConfigParser.cpp
)

# 如果使用内置lodepng
if(USE_BUILTIN_LODEPNG)
    list(APPEND SOURCES src/lodepng.cpp)
endif()

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    ${CAIRO_LIBRARIES}
    ${FREETYPE_LIBRARIES}
)

if(NOT USE_BUILTIN_LODEPNG)
    target_link_libraries(${PROJECT_NAME} ${LODEPNG_LIB})
endif()

# 编译选项
target_compile_options(${PROJECT_NAME} PRIVATE -O3 -Wall -Wextra)

# 安装
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
