# CMake minimum version >= 2.8.12
#cmake  -DCMAKE_BUILD_TYPE=Debug
#cmake  -DCMAKE_BUILD_TYPE=Release
cmake_minimum_required(VERSION 2.8.12)

#project(zyslfch)

#solarflare 通道
set(LIB_SLFCH_NAME zyslfch)
# 设置编译选项
#add_definitions("--std=c++11 -Wall -Wno-nonnull -Wno-reorder -D_LINUX -DLINUX")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11 -Wall -Wno-nonnull -Wno-reorder -D_LINUX -DLINUX -lpthread -lrt -ldl -liconv")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g ${CMAKE_CXX_FLAGS} -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 ${CMAKE_CXX_FLAGS}")


# common目录
set(COMMON_ROOT ../common)
# open onload目录
set(ZF_ROOT /usr/include/zf)

# 设置库输出目录
set(LIBRARY_OUTPUT_PATH ./lib/)



# 头文件目录
include_directories(${COMMON_ROOT}/include 
                    ${ZF_ROOT} 
                    ./include
)

# 编译依赖的子目录文件
# add_subdirectory(${COMMON_ROOT}/)
aux_source_directory(${COMMON_ROOT}/src COMMON_SRC)
#add_library(zycommon STATIC ${COMMON_SRC})
aux_source_directory(./src SLFCH_SRC)


# 设置链接库--绝对路径
set(EFVI_LIB_PATH /usr/lib64/)
set(ZF_LIB_PATH /usr/lib64/zf/debug)

#/usr/lib64/libciul1.a
#/usr/lib64/zf/debug/libonload_zf_static.a
#/usr/lib64/zf/debug/libonload_zf.so


# 源编译目录下的所有源文件
add_library(${LIB_SLFCH_NAME} STATIC ${COMMON_SRC} ${SLFCH_SRC})


# ld 生成目标--绝对路径，依赖顺序，依赖层次由浅到深，最基础的放最后
target_link_libraries(${LIB_SLFCH_NAME}  
                      ${ZF_LIB_PATH}/libonload_zf_static.a 
					  ${EFVI_LIB_PATH}/libciul1.a
)
#					  ${ZF_LIB_PATH}/libonload_zf.so
#)