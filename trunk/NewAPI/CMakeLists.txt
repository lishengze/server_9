# NewAPI 工程 CMakeLists（编排层）
#
# 支持两种编译方式：
#   1. 独立编译：cd trunk/NewAPI && cmake . && make
#   2. 通过根 CMakeLists 编译：由 api_trunk/CMakeLists.txt add_subdirectory 引入
#
# 判定机制（与 grc_trunk 相同）：
#   通过根 CMakeLists 编译时，BUILD_SUM_COUNT 已定义，跳过全局配置；
#   独立编译时，BUILD_SUM_COUNT 未定义，设置完整全局配置。
#
# 构建产物：
#   - common    → lib/liblbcommon.a（lb_common 基础库）
#   - gone/api  → lib/liblbapi.so（交易 API 动态库）
#   - solarflare→ lib/libzyslfch.a（可选，需 Solarflare SDK）
#
# 说明：
#   - fgw（FGW 网关程序）依赖外部 odbc / third_odbc，本工程不依赖外部库，默认不启用。
#   - solarflare（TCPDirect 内核旁路）依赖外部 Solarflare SDK，默认探测，未找到即禁用。

cmake_minimum_required(VERSION 3.10)

# ================= 独立编译时的全局配置 =================
if (NOT DEFINED BUILD_SUM_COUNT)
    project(NewAPI)
    set(BUILD_SUM_COUNT 0)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "Suppress dev warnings")

    # 默认构建类型
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug" FORCE)
    endif()
    message(STATUS "  Compile Type:  ${CMAKE_BUILD_TYPE}")
    message(STATUS "  Platform: "    ${CMAKE_CXX_COMPILER_ID})
    message(STATUS "  Gcc Version: " ${CMAKE_CXX_COMPILER_VERSION})

    # C++ 标准与通用设置
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_VERBOSE_MAKEFILE ON)

    # 编译优化 FLAGS（低延迟交易场景）
    set(CMAKE_CXX_FLAGS "--std=c++11 -march=native -mtune=native -mprefer-vector-width=256 -mpopcnt -m64 -ffast-math -frename-registers -mno-tls-direct-seg-refs -Wall -Wno-nonnull -Wno-reorder -Wpointer-arith -D_LINUX -DLINUX")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -fno-optimize-sibling-calls -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS} -D_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fomit-frame-pointer -funroll-loops -ffunction-sections -fdata-sections -fno-unwind-tables ${CMAKE_CXX_FLAGS} -DNDEBUG")

    # 输出目录
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# ================= SolarFlare 检查与可选编译 =================
option(ENABLE_SOLARFLARE "启用 Solarflare TCPDirect 支持" OFF)
set(SOLARFLARE_ROOT "" CACHE PATH "Solarflare SDK 路径 (默认从环境推导)")

# 查找 Solarflare 头文件和库
if(ENABLE_SOLARFLARE AND NOT SOLARFLARE_ROOT)
    if(EXISTS /usr/include/zf)
        set(SOLARFLARE_ROOT /usr)
    elseif(EXISTS /opt/solarflare)
        set(SOLARFLARE_ROOT /opt/solarflare)
    endif()
endif()

if(ENABLE_SOLARFLARE AND SOLARFLARE_ROOT)
    message(STATUS "Solarflare 模式已启用，路径：${SOLARFLARE_ROOT}")
    add_definitions(-DHAS_TCPDIRECT)
    set(HAS_SOLARFLARE TRUE)
else()
    message(STATUS "Solarflare 模式已禁用（未检测到 SDK 或未开启）")
    set(HAS_SOLARFLARE FALSE)
endif()

# ================= 添加子模块 =================
add_subdirectory(common)

# solarflare 仅在检测到 Solarflare SDK 时才编译
if(HAS_SOLARFLARE)
    add_subdirectory(solarflare)
else()
    message(STATUS "solarflare 模块跳过（未检测到 Solarflare SDK）")
endif()

add_subdirectory(gone/api)

add_dependencies(lbapi lbcommon) # 确保 lbcommon 先编译

message(STATUS "NewAPI 项目构建完成!")
message(STATUS "  - 静态库：${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblbcommon.a")
message(STATUS "  - 动态库：${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblbapi.so")