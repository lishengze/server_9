# gone/api CMakeLists.txt（修复版）
# 
# 输出：lib/liblbapi.so（动态库）
# 依赖：common/lib/liblbcommon.a（静态库）
#
# 编译步骤:
#   1. cd ../.. && mkdir build && cd build
#   2. cmake -DCMAKE_BUILD_TYPE=Release ..
#   3. make -j4
#   4. lib/liblbapi.so 被生成
#

cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wno-unused-result -Wno-unused-parameter -D__linux__)

set(CMAKE_CXX_FLAGS "--std=c++17 -march=native -mtune=native -mprefer-vector-width=256 -mpopcnt -m64 -ffast-math -frename-registers -mno-tls-direct-seg-refs -Wall -Wno-nonnull -Wno-reorder -Wpointer-arith -D_LINUX -DLINUX")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -fno-optimize-sibling-calls -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fomit-frame-pointer -funroll-loops -ffunction-sections -fdata-sections -fno-unwind-tables ${CMAKE_CXX_FLAGS} -DNDEBUG")

# ================= 输出目录 =================
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ================= 头文件路径 =================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include                # 公开 API 头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/src                    # API 内部头文件
    ${CMAKE_SOURCE_DIR}/common/include                 # lbcommon 基础库
    ${CMAKE_CURRENT_SOURCE_DIR}/../../gone             # gone 公共消息定义 (g1msghead.h 等)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../gone/include
)

# Solarflare 支持（如果有）
if(HAS_SOLARFLARE OR EXISTS /usr/include/zf)
    include_directories(${CMAKE_SOURCE_DIR}/solarflare)
    add_compile_options(-DHAS_TCPDIRECT)
endif()

# ================= 源文件 =================
file(GLOB API_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# ================= 构建目标：动态库 =================
add_library(lbapi SHARED ${API_SRC})

# 链接 lbcommon 静态库（通过 CMake 自动查找，不硬编码路径）
target_link_libraries(lbapi PRIVATE lbcommon)

# 标准库 / 系统库
target_link_libraries(lbapi PRIVATE pthread dl)

message(STATUS "API 库构建完成：liblbapi.so")
