message("/n+++++++++++++++++++++++++ API MAIN CMake [START] +++++++++++++++++++++++++")
cmake_minimum_required(VERSION 3.10)
project(API_MAIN)

# 默认构建类型为 Debug，命令行可覆盖为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug" FORCE)
endif()

##################### 编译选项的通用性设置 ##########
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON) ## 解决可能出现的 RPATH 问题

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

message(STATUS "  Compile Type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: "    ${CMAKE_CXX_COMPILER_ID})
message(STATUS "  Gcc Version: " ${CMAKE_CXX_COMPILER_VERSION})

####################### 编译选项的通用性设置 ##########
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# C++ 标准统一为 17（当前框架基于 lb_common + C++17）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 通用编译选项（作用于所有子模块）
add_compile_options(-Wall -Wextra -Wno-unused-result -Wno-unused-parameter -D__linux__)

# 全局基础 FLAGS（低延迟交易场景的编译优化）
set(CMAKE_CXX_FLAGS "--std=c++17 -march=native -mtune=native -mprefer-vector-width=256 -mpopcnt -m64 -ffast-math -frename-registers -mno-tls-direct-seg-refs -Wall -Wno-nonnull -Wno-reorder -Wpointer-arith -D_LINUX -DLINUX")

# Debug/Release 配置
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -fno-optimize-sibling-calls -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fomit-frame-pointer -funroll-loops -ffunction-sections -fdata-sections -fno-unwind-tables ${CMAKE_CXX_FLAGS} -DNDEBUG")

# 链接器优化
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,-z,relro,-z,now")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,-z,relro,-z,now")

# 位置无关代码（PIC）设置：Windows 关闭，Linux 开启
if (${WIN32})
    set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
    message("WIN32, CMAKE_POSITION_INDEPENDENT_CODE OFF")
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    message("NOT WIN32, CMAKE_POSITION_INDEPENDENT_CODE ON")
endif()

###################### 输出目录 ######################
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###################### 子工程源码目录 ######################
set(NEWAPI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/trunk/NewAPI)
if(NOT EXISTS ${NEWAPI_ROOT})
    message(FATAL_ERROR "Can not find NewAPI 工程目录: ${NEWAPI_ROOT}")
endif()

###################### 添加子项目（按构建顺序） ######################
add_subdirectory(${NEWAPI_ROOT} newapi)   # NewAPI 交易 API 框架（lbcommon + lbapi）

message("整个工程当前 共编译 ${BUILD_SUM_COUNT} 个文件")

message("/n+++++++++++++++++++++++++ MAIN CMake [END] +++++++++++++\n++++++++++++")