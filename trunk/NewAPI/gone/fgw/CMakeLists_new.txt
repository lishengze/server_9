# gone/fgw CMakeLists.txt（修复版）
# 
# 输出：bin/fgw（可执行程序）
# 依赖：common/lib/liblbcommon.a + third_odbc（可选）
#
# 编译步骤:
#   1. cd ../.. && mkdir build && cd build
#   2. cmake -DCMAKE_BUILD_TYPE=Release ..
#   3. make -j4
#   4. bin/fgw 被生成
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
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ================= 头文件路径 =================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src                    # fgw 头文件
    ${CMAKE_SOURCE_DIR}/common/include                 # lbcommon 基础库
    ${CMAKE_SOURCE_DIR}/gone                           # gone 公共消息定义
)

# ================= 源文件 =================
file(GLOB Fgw_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

if(Fgw_SRC)
    add_executable(fgw ${Fgw_SRC})

    # 链接 lbcommon 静态库（通过 CMake 自动查找）
    target_link_libraries(fgw PRIVATE lbcommon)

    # third_odbc 支持（可选）
    if(EXISTS ${CMAKE_SOURCE_DIR}/third_odbc)
        add_subdirectory(${CMAKE_SOURCE_DIR}/third_odbc ${CMAKE_BINARY_DIR}/third_odbc)
        target_link_libraries(fgw PRIVATE third_odbc)
    else()
        message(WARNING "third_odbc 未找到，fgw 可能无法运行（需要数据库功能）")
    endif()

    # 标准库 / 系统库
    target_link_libraries(fgw PRIVATE pthread dl odbc)

    message(STATUS "FGW 程序构建完成：fgw")
endif()
