# common 库 CMakeLists.txt（修复版）
# 
# 编译步骤:
#   1. mkdir build && cd build
#   2. cmake -DCMAKE_BUILD_TYPE=Release ..
#   3. make -j4
#   4. lib/liblbcommon.a 被生成
#

cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Linux 环境配置
set(CMAKE_SYSTEM_NAME Linux)

add_compile_options(-Wall -Wextra -Wno-unused-result -Wno-unused-parameter -D__linux__)

# ================= 编译选项配置 =================
set(CMAKE_CXX_FLAGS "--std=c++17 -march=native -mtune=native -mprefer-vector-width=256 -mpopcnt -m64 -ffast-math -frename-registers -mno-tls-direct-seg-refs -Wall -Wno-nonnull -Wno-reorder -Wpointer-arith -D_LINUX -DLINUX")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -fno-optimize-sibling-calls -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fomit-frame-pointer -funroll-loops -ffunction-sections -fdata-sections -fno-unwind-tables ${CMAKE_CXX_FLAGS} -DNDEBUG")

# 链接器优化
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-lpthread -lrt -ldl")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,-z,relro,-z,now -lpthread -lrt -ldl")

set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "-lpthread -lrt -ldl")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,-z,relro,-z,now -lpthread -lrt -ldl")

# ================= 输出目录配置 =================
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# 收集源文件
file(GLOB COMMON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# 编译为静态库
add_library(lbcommon STATIC ${COMMON_SRC})
set_target_properties(lbcommon PROPERTIES POSITION_INDEPENDENT_CODE ON)

# 头文件路径
target_include_directories(lbcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Solarflare 库查找（可选）
option(HAS_SOLARFLARE "是否启用了 Solarflare TCPDirect" OFF)

if(HAS_SOLARFLARE)
    find_library(ZF_LIB NAMES zf onload_zf_static onload_zf PATHS /opt/solarflare/lib /usr/local/lib /usr/lib64/zf/debug NO_DEFAULT_PATH)
    if(ZF_LIB)
        target_link_libraries(lbcommon PUBLIC ${ZF_LIB})
        target_include_directories(lbcommon PUBLIC /opt/solarflare/include /usr/include/zf)
        add_compile_options(-DHAS_TCPDIRECT)
        message(STATUS "Solarflare 库已链接：${ZF_LIB}")
    else()
        message(WARNING "Solarflare 未找到，将不使用 tcpdirect 功能")
    endif()
endif()

# 系统库
target_link_libraries(lbcommon PUBLIC pthread rt dl)

message(STATUS "Common 库构建完成：liblbcommon.a")
