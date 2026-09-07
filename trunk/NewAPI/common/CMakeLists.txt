# common 库 CMakeLists.txt
#
# lb_common 基础库：静态库 liblbcommon.a
# 全局编译选项 / 输出目录已在根 CMakeLists.txt 统一设置。
#
# 编译步骤:
#   1. cd build && cmake ..
#   2. make -j4
#   3. lib/liblbcommon.a 被生成

cmake_minimum_required(VERSION 3.10)

# ================= 源文件 =================
file(GLOB COMMON_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# ================= 构建目标：静态库 =================
add_library(lbcommon STATIC ${COMMON_SRC})
set_target_properties(lbcommon PROPERTIES POSITION_INDEPENDENT_CODE ON)

# 头文件路径
target_include_directories(lbcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 系统库
target_link_libraries(lbcommon PUBLIC pthread rt dl)

message(STATUS "Common 库构建完成：liblbcommon.a")