# solarflare/CMakeLists.txt
#
# 编译 Solarflare TCPDirect 支持库（可选模块）：静态库 libzyslfch.a
# 如果系统没有安装 Solarflare SDK，将跳过此模块。
#
# 支持两种编译方式：
#   1. 独立编译：cd solarflare && cmake . && make
#   2. 通过父模块编译：由 trunk/NewAPI 通过 add_subdirectory 引入
#
# 判定机制（与 grc_trunk 相同）：
#   通过父模块编译时，BUILD_SUM_COUNT 已定义，跳过全局配置；
#   独立编译时，BUILD_SUM_COUNT 未定义，设置完整全局配置。

cmake_minimum_required(VERSION 3.10)

# ================= 独立编译时的全局配置 =================
if (NOT DEFINED BUILD_SUM_COUNT)
    project(zyslfch)
    set(BUILD_SUM_COUNT 0)
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "Suppress dev warnings")

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Release" FORCE)
    endif()

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_VERBOSE_MAKEFILE ON)

    set(CMAKE_CXX_FLAGS "--std=c++17 -Wall -Wno-nonnull -Wno-reorder -D_LINUX -DLINUX")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g ${CMAKE_CXX_FLAGS} -D_DEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS}")

    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# ================= Solarflare 路径 =================
set(ZF_ROOT "" CACHE PATH "Solarflare include 路径")

# 自动检测
if(NOT ZF_ROOT)
    if(EXISTS /usr/include/zf)
        set(ZF_ROOT /usr/include)
    elseif(EXISTS /opt/solarflare/include)
        set(ZF_ROOT /opt/solarflare/include)
    endif()
endif()

if(NOT ZF_ROOT OR NOT EXISTS ${ZF_ROOT})
    message(WARNING "未找到 Solarflare include 目录：${ZF_ROOT}, 将跳过 solarflare 模块编译")
    return()
endif()

# ================= 依赖的 common 头文件路径 =================
set(COMMON_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../common)

include_directories(
    ${COMMON_ROOT}/include
    ${ZF_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ================= 源文件 =================
aux_source_directory(${COMMON_ROOT}/src COMMON_SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SLFCH_SRC)

if(NOT COMMON_SRC OR NOT SLFCH_SRC)
    message(WARNING "Solarflare 模块缺少源文件")
    return()
endif()

# ================= 库配置 =================
set(LIB_SLFCH_NAME zyslfch)

add_library(${LIB_SLFCH_NAME} STATIC ${COMMON_SRC} ${SLFCH_SRC})
set_target_properties(${LIB_SLFCH_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# 查找并链接 Solarflare 库
find_library(ZF_LIB NAMES onload_zf_static onload_zf PATHS /usr/lib64/zf/debug /usr/lib64/zf /opt/solarflare/lib64/zf NO_DEFAULT_PATH)
find_library(CIUL_LIB NAMES ciul1 PATHS /usr/lib64 NO_DEFAULT_PATH)

if(NOT ZF_LIB OR NOT CIUL_LIB)
    message(FATAL_ERROR "未找到 Solarflare 静态库:\n  ZF_LIB: ${ZF_LIB}\n  CIUL_LIB: ${CIUL_LIB}\n请确认已安装 Solarflare SDK 并设置正确路径")
endif()

target_link_libraries(${LIB_SLFCH_NAME} ${ZF_LIB} ${CIUL_LIB})

# ================= 统计编译文件数 =================
list(LENGTH COMMON_SRC COMMON_COUNT)
list(LENGTH SLFCH_SRC SLFCH_COUNT)
math(EXPR SOURCE_COUNT "${COMMON_COUNT} + ${SLFCH_COUNT}")
if (DEFINED BUILD_SUM_COUNT)
    math(EXPR BUILD_SUM_COUNT "${BUILD_SUM_COUNT} + ${SOURCE_COUNT}")
    set(BUILD_SUM_COUNT ${BUILD_SUM_COUNT} PARENT_SCOPE)
endif()

message(STATUS "Solarflare 库构建完成：lib${LIB_SLFCH_NAME}.a（${SOURCE_COUNT} 个文件）")