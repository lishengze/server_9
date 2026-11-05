# perf_client CMakeLists.txt
#
# 构建目标：perf_client（可执行文件）
# 功能：FTE 委托通路性能测试（匀速发单，统计发送结果）
# 依赖：libg1api.so（被测对象）、pthread、dl、sched
#
# 编译方式：
#   独立编译：mkdir build && cd build && cmake .. && make
#   随 API 编译：从项目根目录执行 cmake -S api -B build_cmake -DBUILD_MOCK=ON

# 设置最小版本
cmake_minimum_required(VERSION 2.8)

# 判断是否由父模块编译
if(NOT BUILD_SUM_COUNT)
  project(perf_client CXX)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-array-bounds -Wno-attributes")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-literal-suffix")

  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-mprefer-vector-width=256 CXX_SUPPORT_PREFER_VECTOR_WIDTH)
  if(CXX_SUPPORT_PREFER_VECTOR_WIDTH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mprefer-vector-width=256")
  endif()
endif()

# 源文件：性能测试主程序 + 指标统计 + CPU 亲和 + JSON 工具
file(GLOB PERF_CLIENT_SRC
  src/main.cpp
  src/perf_client.cpp
  src/metric_stats.cpp
  src/cpu_affinity.cpp
)

file(GLOB JSON_UTILS_SRC
  ../include/json_utils.cpp
)

# API 头文件路径（libg1api.so 的接口定义）
set(API_INCLUDE_DIR ../../include)
set(API_SRC_DIR ../src)

# 可执行文件
add_executable(perf_client ${PERF_CLIENT_SRC} ${JSON_UTILS_SRC})

# 头文件路径
target_include_directories(perf_client PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/${API_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/${API_SRC_DIR}
)

# 链接库
if(TARGET g1api)
  target_link_libraries(perf_client g1api pthread dl)
else()
  find_library(G1API_LIB NAMES g1api
    HINTS "${CMAKE_BINARY_DIR}/lib" "${CMAKE_CURRENT_SOURCE_DIR}/../../lib")
  if(NOT G1API_LIB)
    message(FATAL_ERROR "找不到 libg1api.so，请先编译 API 或指定 -DG1API_LIB=<完整路径>")
  endif()
  target_link_libraries(perf_client ${G1API_LIB} pthread dl)
endif()

# 输出目录
if(NOT BUILD_SUM_COUNT)
  set_target_properties(perf_client PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()
