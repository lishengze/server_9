# api 模块 CMakeLists — 独立可编译
#
# 输出：api/lib/liblbapi.so（动态库）
# 依赖：common/lib/liblbcommon.a（静态库，根目录或独立编译都需 pre-built）

cmake_minimum_required(VERSION 3.10)
project(api LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# cmake 生成 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 警告
add_compile_options(-Wall -Wextra -Wno-unused-result -Wno-unused-parameter -D__linux__)

# 全局基础 FLAGS
set(CMAKE_CXX_FLAGS "--std=c++17 -march=native -mtune=native -mprefer-vector-width=256 -mpopcnt -m64 -ffast-math -frename-registers -mno-tls-direct-seg-refs -Wall -Wno-nonnull -Wno-reorder -Wpointer-arith -D_LINUX -DLINUX")

# Debug/Release
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -fno-optimize-sibling-calls -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fomit-frame-pointer -funroll-loops -ffunction-sections -fdata-sections -fno-unwind-tables ${CMAKE_CXX_FLAGS} -DNDEBUG")

# ================= 输出目录 =================
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# ================= 头文件路径 =================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include                # 公开 API 头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/src                    # API 内部头文件
    ${CMAKE_SOURCE_DIR}/../common/include              # lbcommon 基础库
    ${CMAKE_SOURCE_DIR}/../solarflare                  # solarflare（tcpdir_stack.h 等）
    ${CMAKE_SOURCE_DIR}/include                        # gone 公共消息定义 (g1msghead.h 等)
)

# ================= 源文件 =================
file(GLOB API_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# ================= 构建目标：动态库 =================
add_library(lbapi SHARED ${API_SRC})

# 链接 lbcommon 静态库
if(EXISTS ${CMAKE_SOURCE_DIR}/../common/lib/liblbcommon.a)
    target_link_libraries(lbapi PRIVATE ${CMAKE_SOURCE_DIR}/../common/lib/liblbcommon.a)
elseif(EXISTS ${CMAKE_SOURCE_DIR}/../common/build/liblbcommon.a)
    target_link_libraries(lbapi PRIVATE ${CMAKE_SOURCE_DIR}/../common/build/liblbcommon.a)
endif()

# 标准库 / 系统库
target_link_libraries(lbapi PRIVATE pthread dl)
