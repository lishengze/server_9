# api 模块 CMakeLists — 独立可编译
#
# 输出：api/lib/libg1api.so    （socket-only 动态库，恒产出）
#       api/lib/libg1api_sf.so （solarflare tcpdirect 变体，检测到开发环境时追加产出）
# 依赖：common/lib/liblbcommon.a（静态库，根目录或独立编译都需 pre-built）
# 编译方法说明：api/doc/BUILD.md

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
    ${CMAKE_CURRENT_SOURCE_DIR}/counter98              # 98 柜台 ATP AGW 协议库 (vendor)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/include              # lbcommon 基础库
    ${CMAKE_CURRENT_SOURCE_DIR}/../../solarflare                  # solarflare（tcpdir_stack.h 等）
    ${CMAKE_CURRENT_SOURCE_DIR}/../include                        # gone 公共消息定义 (g1msghead.h 等)
)

# ================= 源文件 =================
file(GLOB G1API_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/counter98/*.cpp)
# factory.cpp / msg_map_agw_external.cpp 剔除: 消息工厂与注册表符号无任何引用
# (组包/解析直接构造消息对象), 且二者依赖原构建环境的传递包含与单 TU 假设
# (缺 <string>、std::hash 特化与使用分离), 保持 counter98/ 目录文件零改动
list(REMOVE_ITEM G1API_SRC ${CMAKE_CURRENT_SOURCE_DIR}/counter98/factory.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/counter98/msg_map_agw_external.cpp)

# ================= Solarflare TCPDirect 检测（零参数自动；g1api_sf 追加目标） =================
# 默认 AUTO：无需任何 -D 参数——检测到 onload/TCPDirect 开发环境则额外产出
# libg1api_sf.so（tcpdirect 变体），未检测到则只产出 libg1api.so（socket-only）。
# libg1api.so 恒产出：配置误选 speed_link_type=tcpdirect 时其在配置校验层与
# factory 层双防线返回 LBAPI_ERR_UNSUPPORT_LINK。
# WITH_TCPDIRECT 仅控制 g1api_sf 的产出: OFF=强制跳过(即使有环境) / AUTO=自动(默认) / ON=强制要求(缺失即报错)
# 官方口径（TCPDirect/ef_vi User Guide, AMD docs.amd.com）:
#   - 编译期检测的是【开发文件】：/usr/include/zf 真头 + libonload_zf* 库（网卡与
#     驱动是运行期概念，编译机无需网卡；运行期 zf_init/建栈失败由引擎 init 报错）
#   - 库形态两者皆官方支持：动态 libonload_zf.so / 静态 libonload_zf_static.a；
#     静态组合需搭配 libciul1.a（ef_vi 用户态库, Onload UG1586 明确背书）→ 按裁决优先静态
set(WITH_TCPDIRECT "AUTO" CACHE STRING "g1api_sf (solarflare tcpdirect) variant: OFF | AUTO | ON")

set(TCPDIRECT_FOUND FALSE)
if(NOT WITH_TCPDIRECT STREQUAL "OFF")
    # 只认系统安装的真头（NO_DEFAULT_PATH + 显式路径，绝不误中仓内 solarflare/zf 桩头）
    find_path(ZF_INCLUDE_DIR zf/zf.h
              PATHS /usr/include /usr/local/include
              NO_DEFAULT_PATH)
    find_library(ZF_STATIC_LIB NAMES onload_zf_static
                 PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib /usr/local/lib
                 NO_DEFAULT_PATH)
    find_library(CIUL_STATIC_LIB NAMES ciul1
                 PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib /usr/local/lib
                 NO_DEFAULT_PATH)
    find_library(ZF_SHARED_LIB NAMES onload_zf
                 PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib /usr/local/lib
                 NO_DEFAULT_PATH)

    if(ZF_INCLUDE_DIR AND ((ZF_STATIC_LIB AND CIUL_STATIC_LIB) OR ZF_SHARED_LIB))
        set(TCPDIRECT_FOUND TRUE)
    endif()

    if(WITH_TCPDIRECT STREQUAL "ON" AND NOT TCPDIRECT_FOUND)
        message(FATAL_ERROR
            "WITH_TCPDIRECT=ON 但未检测到 TCPDirect 开发环境（需 /usr/include/zf/zf.h 与 "
            "libonload_zf_static.a+libciul1.a 或 libonload_zf.so）。请安装 onload/TCPDirect "
            "或将 WITH_TCPDIRECT 置为 OFF/AUTO。")
    endif()
endif()

# ================= 构建目标 1：libg1api.so（socket-only，恒产出） =================
add_library(g1api SHARED ${G1API_SRC})

if(TCPDIRECT_FOUND)
    if(ZF_STATIC_LIB AND CIUL_STATIC_LIB)
        set(ZF_LINK_LIBS ${ZF_STATIC_LIB} ${CIUL_STATIC_LIB}) # 静态优先（用户裁决）
    else()
        set(ZF_LINK_LIBS ${ZF_SHARED_LIB})
    endif()

    # ================= 构建目标 2：libg1api_sf.so（tcpdirect 变体，同源 + 宏 + zf 封装层） =================
    set(G1API_SF_SRC ${G1API_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/../../solarflare/tcpdir_stack.cpp)
    add_library(g1api_sf SHARED ${G1API_SF_SRC})
    target_compile_definitions(g1api_sf PRIVATE HAS_TCPDIRECT=1)
    # 系统 zf 真头插到该目标 -I 最前：压过 ../solarflare（其 zf/ 子目录是仅供无环境编译的桩头）
    target_include_directories(g1api_sf BEFORE PRIVATE ${ZF_INCLUDE_DIR})
    target_link_libraries(g1api_sf PRIVATE ${ZF_LINK_LIBS})

    # 链接 lbcommon 静态库（sf 变体同需）
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../common/lib/liblbcommon.a)
        target_link_libraries(g1api_sf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../common/lib/liblbcommon.a)
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../common/build/liblbcommon.a)
        target_link_libraries(g1api_sf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../common/build/liblbcommon.a)
    endif()
    target_link_libraries(g1api_sf PRIVATE pthread dl)

    message(STATUS "TCPDirect: ENABLED → 额外产出 libg1api_sf.so (zf_inc=${ZF_INCLUDE_DIR}, libs=${ZF_LINK_LIBS})")
else()
    message(STATUS "TCPDirect: DISABLED → 仅产出 libg1api.so (socket-only, tcpdirect 选型双防线返回 UNSUPPORT_LINK)")
endif()

# 链接 lbcommon 静态库
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../common/lib/liblbcommon.a)
    target_link_libraries(g1api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../common/lib/liblbcommon.a)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../common/build/liblbcommon.a)
    target_link_libraries(g1api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../common/build/liblbcommon.a)
endif()

# 标准库 / 系统库
target_link_libraries(g1api PRIVATE pthread dl)

# counter98 vendor 告警抑制（vendor 目录零改动裁决，仅在构建侧关闭该类检查）：
# string_util.cpp:125/224 对 size_t 判 n<0 恒假（-Wtype-limits），属 vendor 既有代码
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/counter98/string_util.cpp
                            PROPERTIES COMPILE_OPTIONS "-Wno-type-limits")
