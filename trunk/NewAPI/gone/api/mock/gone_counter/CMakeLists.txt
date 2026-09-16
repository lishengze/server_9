# gone_counter_mock CMakeLists.txt
#
# 构建目标：gone_counter_mock（可执行文件）
# 功能：模拟 GOne（FPGA）柜台服务端
#   - 接收 g1 协议连接（GW 链接 + Core 链接）
#   - 处理：证券信息请求、登录、心跳、委托、撤单
# 依赖：pthread（无外部库）
#
# 编译方式：
#   独立编译：mkdir build && cd build && cmake .. && make
#   父模块编译：项目根目录 ./build.sh -DBUILD_MOCK=ON

# 设置最小版本
cmake_minimum_required(VERSION 2.8)

# 判断是否由父模块编译
if(NOT BUILD_SUM_COUNT)
  project(gone_counter_mock CXX)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-array-bounds -Wno-attributes")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-literal-suffix")

  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-mprefer-vector-width=256 CXX_SUPPORT_PREFER_VECTOR_WIDTH)
  if(CXX_SUPPORT_PREFER_VECTOR_WIDTH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mprefer-vector-width=256")
  endif()
endif()

# 源文件
file(GLOB GONE_COUNTER_SRC
  src/main.cpp
  src/gone_counter_server.cpp
  src/client_session.cpp
  src/account_manager.cpp
  src/message_parser.cpp
  src/session_registry.cpp
)

file(GLOB JSON_UTILS_SRC
  ../include/json_utils.cpp
)

# g1 协议头文件路径（g1msghead.h / g1trademsg.h）
set(G1_INCLUDE_DIR ../../../include)

# 可执行文件
add_executable(gone_counter_mock ${GONE_COUNTER_SRC} ${JSON_UTILS_SRC})

# 头文件路径
target_include_directories(gone_counter_mock PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/${G1_INCLUDE_DIR}
)

# 链接库
target_link_libraries(gone_counter_mock pthread)

# 输出目录
if(NOT BUILD_SUM_COUNT)
  set_target_properties(gone_counter_mock PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()
