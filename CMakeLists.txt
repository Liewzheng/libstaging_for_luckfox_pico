cmake_minimum_required(VERSION 3.16)
project(libstaging VERSION 1.0.0 LANGUAGES C)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -Wall -Wextra")

# ============================================================================
# 交叉编译工具链配置
# ============================================================================

# 检查是否从外部传入了 TOOLCHAIN_PREFIX
if(DEFINED TOOLCHAIN_PREFIX AND EXISTS "${TOOLCHAIN_PREFIX}/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
    message(STATUS "Using external toolchain: ${TOOLCHAIN_PREFIX}")
    
    # ARM 架构特定标志（如果需要可以在这里添加）
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=hard")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}/bin/arm-rockchip830-linux-uclibcgnueabihf-g++")
    set(CMAKE_AR "${TOOLCHAIN_PREFIX}/bin/arm-rockchip830-linux-uclibcgnueabihf-ar")
    set(CMAKE_STRIP "${TOOLCHAIN_PREFIX}/bin/arm-rockchip830-linux-uclibcgnueabihf-strip")
    
    # 设置交叉编译标志
    set(CMAKE_CROSSCOMPILING TRUE)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    
    message(STATUS "Configuring for cross-compilation with toolchain: ${TOOLCHAIN_PREFIX}")
else()
    message(STATUS "Using native compilation (no external toolchain specified)")
endif()


# 包含目录 - 仅包含 include 目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# 收集源文件 - 仅从 source 目录
file(GLOB_RECURSE LIBSTAGING_SOURCES
    ${CMAKE_SOURCE_DIR}/source/*.c
)

# 检查是否找到源文件
if(NOT LIBSTAGING_SOURCES)
    message(WARNING "No source files found in ${CMAKE_SOURCE_DIR}/source/")
    message(STATUS "Looking for files matching: ${CMAKE_SOURCE_DIR}/source/*.c")
endif()

# 打印找到的源文件
message(STATUS "Found source files:")
foreach(src_file ${LIBSTAGING_SOURCES})
    message(STATUS "  ${src_file}")
endforeach()

# 创建 libstaging.so 动态库
add_library(staging SHARED
    ${LIBSTAGING_SOURCES}
)

# 设置动态库的输出名称和版本
set_target_properties(staging PROPERTIES
    OUTPUT_NAME "staging"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    C_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)

# 如果父项目设置了统一的输出目录，使用父项目的设置
if(CMAKE_LIBRARY_OUTPUT_DIRECTORY_FOR_SUBMODULES)
    set_target_properties(staging PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_FOR_SUBMODULES}"
    )
elseif(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(staging PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    )
else()
    set_target_properties(staging PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endif()

# 为动态库设置包含目录
target_include_directories(staging PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 为动态库链接必要的系统库
target_link_libraries(staging
    pthread
    m
)

# 设置动态库的编译标志
target_compile_options(staging PRIVATE
    -fPIC
)

# 导出符号定义 - 让所有符号默认可见
target_compile_definitions(staging PRIVATE
    STAGING_EXPORTS=1
)

# 打印配置信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Library Directory: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "Source Directory: ${CMAKE_SOURCE_DIR}/source/")
message(STATUS "Include Directory: ${CMAKE_SOURCE_DIR}/include/")

# 安装配置
install(TARGETS staging
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 安装头文件 - 安装 include 目录下的所有头文件
file(GLOB HEADER_FILES ${CMAKE_SOURCE_DIR}/include/*.h)
install(FILES ${HEADER_FILES}
    DESTINATION include
)

# 创建 pkg-config 文件
configure_file(
    "${CMAKE_SOURCE_DIR}/libstaging.pc.in"
    "${CMAKE_BINARY_DIR}/libstaging.pc"
    @ONLY
)

install(FILES "${CMAKE_BINARY_DIR}/libstaging.pc"
    DESTINATION lib/pkgconfig
)

# 添加自定义目标来显示库信息
add_custom_target(library-info
    COMMAND echo "=== Library Information ==="
    COMMAND echo "Library: libstaging.so.${PROJECT_VERSION}"
    COMMAND echo "Location: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    COMMAND echo "Headers: ${CMAKE_SOURCE_DIR}/include/"
    COMMAND echo "Sources: ${CMAKE_SOURCE_DIR}/source/"
    COMMAND echo "=== Build Commands ==="
    COMMAND echo "Build library: make staging"
    COMMAND echo "Build all: make"
    DEPENDS staging
)

# 添加自定义目标来检查动态库
add_custom_target(check-library
    COMMAND echo "Checking libstaging.so..."
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Library file:"
    COMMAND ls -la ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libstaging.so*
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Library dependencies:"
    COMMAND ldd ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libstaging.so || echo "ldd not available (cross-compiled)"
    COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan "Library symbols:"
    COMMAND nm -D ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libstaging.so | head -20 || echo "nm not available"
    DEPENDS staging
)

# 添加自定义目标来列出目录结构
add_custom_target(show-structure
    COMMAND echo "=== Project Structure ==="
    COMMAND echo "Source files in source/:"
    COMMAND find ${CMAKE_SOURCE_DIR}/source -name "*.c" -type f || echo "No .c files found in source/"
    COMMAND echo ""
    COMMAND echo "Header files in include/:"
    COMMAND find ${CMAKE_SOURCE_DIR}/include -name "*.h" -type f || echo "No .h files found in include/"
)

# 设置默认目标
add_custom_target(all-info ALL
    COMMAND echo "=== libstaging Build Configuration ==="
    COMMAND echo "Project: ${PROJECT_NAME}"
    COMMAND echo "Version: ${PROJECT_VERSION}"
    COMMAND echo "Source Directory: ${CMAKE_SOURCE_DIR}/source/"
    COMMAND echo "Include Directory: ${CMAKE_SOURCE_DIR}/include/"
    COMMAND echo "Output Library: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libstaging.so"
    COMMAND echo ""
    DEPENDS staging
)