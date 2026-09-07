# gone/api CMakeLists.txt
#
# 交易 API 动态库：lib/liblbapi.so
# 依赖：common 的 liblbcommon.a（静态库）
#
# 支持两种编译方式：
#   1. 独立编译：cd gone/api && cmake . && make
#      （会自动 add_subdirectory 构建依赖的 common 库）
#   2. 通过父模块编译：由 trunk/NewAPI 通过 add_subdirectory 引入
#
# 判定机制（与 grc_trunk 相同）：
#   通过父模块编译时，BUILD_SUM_COUNT 已定义，跳过全局配置，沿用父模块设置；
#   独立编译时，BUILD_SUM_COUNT 未定义，设置完整全局配置并构建依赖库。

cmake_minimum_required(VERSION 3.10)

# ================= 独立编译时的全局配置 =================
if (NOT DEFINED BUILD_SUM_COUNT)
    project(lbapi)
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
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_VERBOSE_MAKEFILE ON)

    # 编译优化 FLAGS（低延迟交易场景）
    set(CMAKE_CXX_FLAGS "--std=c++17 -march=native -mtune=native -mprefer-vector-width=256 -mpopcnt -m64 -ffast-math -frename-registers -mno-tls-direct-seg-refs -Wall -Wno-nonnull -Wno-reorder -Wpointer-arith -D_LINUX -DLINUX")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -fno-optimize-sibling-calls -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS} -D_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fomit-frame-pointer -funroll-loops -ffunction-sections -fdata-sections -fno-unwind-tables ${CMAKE_CXX_FLAGS} -DNDEBUG")

    # 输出目录
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    # 独立编译时，先构建依赖的 common 库（lbcommon）
    # 注意：common 目录不在本模块子目录下，必须显式指定 binary 目录
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../common common_build)
endif()

# ================= Solarflare 检查与可选编译 =================
# 通过父模块编译时，HAS_SOLARFLARE 已由父模块（trunk/NewAPI）检测确定，直接沿用；
# 独立编译时，此处自行检测 Solarflare SDK 并决定是否启用 TCPDirect 支持。
if(NOT DEFINED HAS_SOLARFLARE)
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
        set(HAS_SOLARFLARE TRUE)
    else()
        message(STATUS "Solarflare 模式已禁用（未检测到 SDK 或未开启）")
        set(HAS_SOLARFLARE FALSE)
    endif()
endif()

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

# ================= 统计编译文件数 =================
list(LENGTH API_SRC SOURCE_COUNT)
if (DEFINED BUILD_SUM_COUNT)
    math(EXPR BUILD_SUM_COUNT "${BUILD_SUM_COUNT} + ${SOURCE_COUNT}")
    set(BUILD_SUM_COUNT ${BUILD_SUM_COUNT} PARENT_SCOPE)
endif()

message(STATUS "API 库构建完成：liblbapi.so（${SOURCE_COUNT} 个文件）")