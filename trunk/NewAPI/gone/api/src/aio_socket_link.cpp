// aio_socket_link - 基于 aio_tcp 的非阻塞 TCP 链接实现 (组合模式)
//
// 所有模板函数实现已移入 aio_socket_link.h 头文件，
// 以确保显式模板实例化时编译器能看到完整定义。
//
// 此 .cpp 文件仅保留以兼容 CMakeLists.txt 的 file(GLOB ...) 收集方式。

#include "aio_socket_link.h"
