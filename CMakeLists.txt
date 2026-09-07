message("/n+++++++++++++++++++++++++ API MAIN CMake [START] +++++++++++++++++++++++++")
cmake_minimum_required(VERSION 3.10)
project(API_MAIN)

# 默认构建类型为 Debug，命令行可覆盖为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug" FORCE)
endif()

##################### 编译选项的通用性设置 ##########
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON) ## 解决可能出现的 RPATH 问题;


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

# 默认构建类型为 Debug，命令行可覆盖为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug" FORCE)
endif()
message(STATUS "  Compile Type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: "    ${CMAKE_CXX_COMPILER_ID})
message(STATUS "  Gcc Version: " ${CMAKE_CXX_COMPILER_VERSION})


####################### 编译选项的通用性设置 ##########
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

message("整个工程当前 共编译 ${BUILD_SUM_COUNT} 个文件")

message("/n+++++++++++++++++++++++++ MAIN CMake [END] +++++++++++++\n++++++++++++")