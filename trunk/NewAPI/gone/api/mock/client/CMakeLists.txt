# mock_client CMakeLists.txt
#
# 构建目标：mock_client（可执行文件，~1MB）
# 功能：加载 liblbapi.so，通过 JSON 配置化测试 FTE 协议完整链路
# 依赖：liblbapi.so（被测对象）、pthread、dl
#
# 编译方式：
#   独立编译：mkdir build && cd build && cmake .. && make
#   父模块编译：项目根目录 ./build.sh -DBUILD_MOCK=ON

# 设置最小版本
cmake_minimum_required(VERSION 2.8)

# 判断是否由父模块编译
if(NOT BUILD_SUM_COUNT)
  project(mock_client CXX)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-array-bounds -Wno-attributes")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-literal-suffix")

  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-mprefer-vector-width=256 CXX_SUPPORT_PREFER_VECTOR_WIDTH)
  if(CXX_SUPPORT_PREFER_VECTOR_WIDTH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mprefer-vector-width=256")
  endif()
endif()

# 源文件：mock_client 核心 + JSON 工具
file(GLOB MOCK_CLIENT_SRC
  src/main.cpp
  src/mock_client.cpp
  src/test_case_runner.cpp
  src/callback_handler.cpp
  src/test_report.cpp
)

file(GLOB JSON_UTILS_SRC
  ../include/json_utils.cpp
)

# API 头文件路径（liblbapi.so 的接口定义）
set(API_INCLUDE_DIR ../../include)
set(API_SRC_DIR ../src)

# 可执行文件
add_executable(mock_client ${MOCK_CLIENT_SRC} ${JSON_UTILS_SRC})

# 头文件路径
target_include_directories(mock_client PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/${API_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/${API_SRC_DIR}
)

# 链接库
# liblbapi.so 提供 api_config::create_config / api_interface::create_instance
# 使用 --allow-shlib-undefined 允许 liblbapi.so 在运行时解析其模板符号
find_library(LBAPI_LIB lbapi HINTS ${CMAKE_BINARY_DIR}/lib)
if(LBAPI_LIB)
  target_link_libraries(mock_client pthread dl ${LBAPI_LIB})
else()
  target_link_libraries(mock_client pthread dl lbapi)
endif()

# 允许共享库的未定义符号（模板实例化在运行时解析）
set_target_properties(mock_client PROPERTIES
  LINK_FLAGS "-Wl,--allow-shlib-undefined"
)

# 输出目录
if(NOT BUILD_SUM_COUNT)
  set_target_properties(mock_client PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()
