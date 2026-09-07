# gone/api CMakeLists.txt
#
# 交易 API 动态库：lib/liblbapi.so
# 依赖：common 的 liblbcommon.a（静态库）
# 全局编译选项 / 输出目录已在根 CMakeLists.txt 统一设置。
#
# 编译步骤:
#   1. cd build && cmake ..
#   2. make -j4
#   3. lib/liblbapi.so 被生成

cmake_minimum_required(VERSION 3.10)

# ================= 头文件路径 =================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include                 # 公开 API 头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/src                     # API 内部头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/include    # lbcommon 基础库
    ${CMAKE_CURRENT_SOURCE_DIR}/../../gone              # gone 公共消息定义
    ${CMAKE_CURRENT_SOURCE_DIR}/../../gone/include      # g1msghead.h / g1trademsg.h / c98msg_tmp.h
)

# ================= 源文件 =================
file(GLOB API_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# TCPDirect（Solarflare）相关源文件仅在启用 Solarflare SDK 时编译。
# 未启用时排除，否则会因缺少 tcpdir_stack.h 等 SDK 头文件而编译失败。
if(NOT HAS_SOLARFLARE)
    list(REMOVE_ITEM API_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tcpdir_link.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tcpdirect_engine.cpp
    )
endif()

# ================= 构建目标：动态库 =================
add_library(lbapi SHARED ${API_SRC})

# 链接 lbcommon 静态库（通过 CMake 目标引用，不硬编码路径）
target_link_libraries(lbapi PRIVATE lbcommon)

# 标准库 / 系统库
target_link_libraries(lbapi PRIVATE pthread dl)

# Solarflare 支持宏（如果启用了 TCPDirect）
if(HAS_SOLARFLARE)
    target_compile_definitions(lbapi PRIVATE HAS_TCPDIRECT)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../solarflare)
endif()

message(STATUS "API 库构建完成：liblbapi.so")