#pragma once

/**
 * @file thread_comm.h
 * @brief 线程通信模块
 *
 * 提供基础的线程创建、管理和信号处理功能
 * 包括线程信号屏蔽、信号处理函数安装等实用工具
 */

#include "comm_sys.h"
#include <pthread.h>

namespace lb_common {

/**
 * @brief 线程函数指针类型定义
 *
 * 定义线程入口函数的指针类型
 *
 * @param[in] arg 传递给线程的参数
 * @return void* 线程返回值
 */
typedef void *(*f_thread_comfunc)(void *arg);

/**
 * @brief 全局信号停止控制变量
 *
 * 用于控制线程的停止状态，-1表示正常运行
 */
extern int32 g_signal_stopctl;

/**
 * @brief 屏蔽线程信号
 *
 * 屏蔽常见的线程中断信号，确保线程稳定运行
 *
 * @note 屏蔽的信号：SIGINT, SIGQUIT, SIGTTOU, SIGTTIN, SIGTSTP, SIGHUP, SIGPIPE
 */
void mask_thread_signal();

/**
 * @brief 安装信号处理函数
 *
 * 安装各种信号的处理函数，包括忽略某些信号和设置停止处理
 *
 * @note 忽略的信号：SIGTTOU, SIGTTIN, SIGTSTP, SIGPIPE, SIGHUP
 * @note 停止处理的信号：SIGABRT, SIGBUS, SIGQUIT, SIGTERM, SIGSTOP, SIGKILL
 */
void install_signal_hand();

/**
 * @brief 创建并运行线程
 *
 * 创建一个新线程并启动执行
 *
 * @param[in] pfunc 线程入口函数指针
 * @param[in] th_arg 传递给线程的参数
 * @return pthread_t 线程ID，失败返回错误码
 */
pthread_t run_thread(f_thread_comfunc pfunc, void *th_arg);

/**
 * @brief 等待线程结束
 *
 * 阻塞等待指定线程结束
 *
 * @param[in] th_id 要等待的线程ID
 */
void wait_thread(pthread_t th_id);

} // namespace lb_common
