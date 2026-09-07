# NewAPI 工程 CMakeLists（编排层）
#
# 由根目录 CMakeLists.txt 作为统一构建入口 add_subdirectory 引入。
# 全局编译选项 / 输出目录 / C++ 标准已在根 CMakeLists.txt 统一设置，此处不再重复。
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

# ================= SolarFlare 检查与可选编译 =================
option(ENABLE_SOLARFLARE "启用 Solarflare TCPDirect 支持" OFF)
set(SOLARFLARE_ROOT "" CACHE PATH "Solarflare SDK 路径 (默认从环境推导)")

# 查找 Solarflare 头文件和库
if(ENABLE_SOLARFLARE AND NOT SOLARFLARE_ROOT)
    # 尝试常见路径
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
# 注意：add_subdirectory 会按顺序执行每个子模块的 CMakeLists
add_subdirectory(common)

# solarflare 仅在检测到 Solarflare SDK 时才编译
if(HAS_SOLARFLARE)
    add_subdirectory(solarflare)
else()
    message(STATUS "solarflare 模块跳过（未检测到 Solarflare SDK）")
endif()

add_subdirectory(gone/api)

message(STATUS "NewAPI 项目构建完成!")
message(STATUS "  - 静态库：${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblbcommon.a")
message(STATUS "  - 动态库：${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblbapi.so")