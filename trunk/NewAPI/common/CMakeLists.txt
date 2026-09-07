# common 库 CMakeLists.txt
#
# lb_common 基础库：静态库 liblbcommon.a
#
# 支持两种编译方式：
#   1. 独立编译：cd common && cmake . && make
#   2. 通过父模块编译：由 trunk/NewAPI 或根 CMakeLists 通过 add_subdirectory 引入
#
# 判定机制（与 grc_trunk 相同）：
#   通过父模块编译时，父模块已定义 BUILD_SUM_COUNT，此处跳过全局配置，沿用父模块设置；
#   独立编译时，BUILD_SUM_COUNT 未定义，此处设置完整的全局配置。

cmake_minimum_required(VERSION 3.10)

# ================= 独立编译时的全局配置 =================
if (NOT DEFINED BUILD_SUM_COUNT)
    project(lbcommon)
    set(BUILD_SUM_COUNT 0)      ## 用于统计整个编译的文件数目
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
endif()

# ================= 源文件 =================
file(GLOB COMMON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# ================= 构建目标：静态库 =================
add_library(lbcommon STATIC ${COMMON_SRC})
set_target_properties(lbcommon PROPERTIES POSITION_INDEPENDENT_CODE ON)

# 头文件路径
target_include_directories(lbcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 系统库
target_link_libraries(lbcommon PUBLIC pthread rt dl)

# ================= 统计编译文件数 =================
list(LENGTH COMMON_SRC SOURCE_COUNT)
if (DEFINED BUILD_SUM_COUNT)
    math(EXPR BUILD_SUM_COUNT "${BUILD_SUM_COUNT} + ${SOURCE_COUNT}")
    set(BUILD_SUM_COUNT ${BUILD_SUM_COUNT} PARENT_SCOPE)
endif()

message(STATUS "Common 库构建完成：liblbcommon.a（${SOURCE_COUNT} 个文件）")