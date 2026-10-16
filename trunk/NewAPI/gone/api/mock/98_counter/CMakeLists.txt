# counter98_mock CMakeLists.txt
#
# 构建目标：counter98_mock（可执行文件）
# 功能：模拟 98 柜台服务端，支持 AGW 登录 / 账户登录 / 心跳 / 委托 / 撤单 / 各类查询
# 依赖：counter98 协议库（直接编译源文件）、lbcommon（静态库）、pthread
#
# 编译方式：
#   独立编译：mkdir build && cd build && cmake .. && make
#   父模块编译：项目根目录 ./build.sh -DBUILD_MOCK=ON

# 设置最小版本
cmake_minimum_required(VERSION 2.8)

# 判断是否由父模块编译
if(NOT BUILD_SUM_COUNT)
  project(counter98_mock CXX)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2 -Wall -Werror -Wno-unused-variable -Wno-unused-but-set-variable -Wno-array-bounds -Wno-attributes")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-literal-suffix")

  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-mprefer-vector-width=256 CXX_SUPPORT_PREFER_VECTOR_WIDTH)
  if(CXX_SUPPORT_PREFER_VECTOR_WIDTH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mprefer-vector-width=256")
  endif()
endif()

# ================= 头文件路径 =================
set(COUNTER98_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../counter98)
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../common)
set(GONE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

# ================= 源文件 =================
# mock 服务端核心
file(GLOB MOCK_98_SRC
  src/main.cpp
  src/counter98_server.cpp
  src/client_session.cpp
  src/account_manager.cpp
)

# JSON 工具
file(GLOB JSON_UTILS_SRC
  ../include/json_utils.cpp
)

# counter98 协议源文件（直接编译，不依赖 liblbapi.so）
file(GLOB COUNTER98_PROTOCOL_SRC
  ${COUNTER98_DIR}/messages_agw_external_nonjson.cpp
  ${COUNTER98_DIR}/root_packet.cpp
  ${COUNTER98_DIR}/msg_map_agw_external.cpp
  ${COUNTER98_DIR}/factory.cpp
  ${COUNTER98_DIR}/string_util.cpp
)

# 可执行文件
add_executable(counter98_mock
  ${MOCK_98_SRC}
  ${JSON_UTILS_SRC}
  ${COUNTER98_PROTOCOL_SRC}
)

# 头文件路径
target_include_directories(counter98_mock PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${COUNTER98_DIR}                         # 98 柜台协议头文件
  ${COMMON_DIR}/include                    # lbcommon 基础库
  ${GONE_DIR}                              # gone 公共定义
  ${GONE_DIR}/include                      # c98msg_tmp.h
)

# 链接库
target_link_libraries(counter98_mock pthread)

# 如果 lbcommon 已构建为 target，直接链接；否则编译需要的 common 源文件
if(TARGET lbcommon)
  target_link_libraries(counter98_mock lbcommon)
else()
  # 独立编译时，编译 common 中需要的源文件（网络字节序转换等）
  # 这里只编译最小依赖：comm_sys.cpp（HostToNetwork/NetworkToHost）
  find_file(COMM_SYS_SRC comm_sys.cpp PATHS ${COMMON_DIR}/src NO_DEFAULT_PATH)
  if(COMM_SYS_SRC)
    target_sources(counter98_mock PRIVATE ${COMM_SYS_SRC})
  endif()
endif()

# counter98 协议导出宏
target_compile_definitions(counter98_mock PRIVATE EXPORT_MACRO MP_EXPORT_DLL)

# 输出目录
if(NOT BUILD_SUM_COUNT)
  set_target_properties(counter98_mock PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
endif()
