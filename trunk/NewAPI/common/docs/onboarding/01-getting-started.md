---
purpose: GETTING-STARTED
audience: AI agents and humans writing their first lbcommon program
source-of-truth: yes (for the working example only; API signatures in include/*.h)
last-reviewed: 2026-08-31
---

# Getting Started：30 行 hello TCP echo

> 本文件给出**最小可运行**的 lbcommon 用法：从业务项目骨架到 `aio_tcp` 客户端/服务器能发送并接收消息。
> 重点：让你看清 lbcommon 的"回调 + epoll 线程 + 缓存策略"骨架怎么拼起来。
> **这不是生产代码**——生产用 `mthread_pool` + 多连接 + 心跳，详见 [05-business-recipes.md](./05-business-recipes.md)。

## 1. 业务项目目录结构

```
projects/
├── common/         ← lbcommon（本仓库）
└── my_app/         ← 业务项目
    ├── CMakeLists.txt
    └── main.cpp
```

## 2. CMakeLists.txt（业务项目）

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_app CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 关键：把 common 作为子目录引入
add_subdirectory(../common ${CMAKE_BINARY_DIR}/lbcommon_build)

# 业务可执行文件
add_executable(my_app main.cpp)

# 链接 lbcommon（自动带入 include 路径与 pthread/rt/dl 依赖）
target_link_libraries(my_app PRIVATE lbcommon)
```

详细 CMake 模板与集成注意见 [02-integration-guide.md](./02-integration-guide.md)。

## 3. main.cpp：hello TCP echo 客户端

下面的客户端连接 `127.0.0.1:9000`，发送 `"hello\n"`，收到 echo 后退出。配套的 echo 服务器在 §5。

```cpp
// my_app/main.cpp
#include <aio_tcp.h>
#include <comm_errno.h>
#include <comm_sock.h>
#include <mthread.h>
#include <cstdio>
#include <cstring>

using namespace lb_common;

// 消息回调：纯虚 deal_msg，返回已处理字节数
class my_recv_cb : public ch_recv_cb<tcp_buf_ch> {
public:
  bool got_reply = false;
  int32 deal_msg(tcp_buf_ch *pch, aio_msg &msg) override {
    printf("收到 %d 字节: %.*s\n", msg.msglen, msg.msglen, msg.pmsg);
    got_reply = true;
    pch->close_ch(0);          // 收到应答即关
    return msg.msglen;          // 返回处理长度
  }
};

// 通道生命周期回调：默认空实现，按需 override
class my_ch_op : public aio_ch_op<tcp_buf_ch> {
public:
  // 连接成功（含异步完成）
  void deal_ch_connect(tcp_buf_ch *pch, csock_addr &localaddr) override {
    printf("已连接，开始发 hello\n");
    const char *hello = "hello\n";
    int32 ret = pch->send_msg(const_cast<char *>(hello), (int32)strlen(hello));
    if (ret < 0) {
      lb_comm_err err; perror_lb(err, ret);
      fprintf(stderr, "send_msg 失败: %s\n", err.err_msg);
      pch->close_ch(ret);
    }
  }
  // 引用归零、真正释放资源时回调
  void deal_ch_closed(tcp_buf_ch *pch, int32 errcode) override {
    printf("连接已关闭 (err=%d)\n", errcode);
  }
};

int main() {
  // 1) 启动接收线程（epoll + 事件队列）
  mthread recv_th;
  recv_th.init_th(/*id*/0, /*maxpoll*/1024,
                  /*eventq_size*/256, /*eventq_loop*/6,
                  /*waitms*/10);
  recv_th.run();

  // 2) 配置地址
  csock_addr remote;
  remote.set_addr("127.0.0.1", 9000);  // 实际 API 见 comm_sock.h

  // 3) 配置 aio_attr
  aio_attr battr{};
  battr.onerecvtimes = 4;
  battr.maxmsglen = 1024;
  battr.buf_size = 4096;
  battr.heartinterval = 0;  // 关心跳

  // 4) 配置 channel_attr
  channel_attr cattr{};
  cattr.family = CHANNEL_FAMILY_IPV4;  // 仅 IPv4

  // 5) 创建 aio_tcp 客户端（最简缓存：aio_recv_buf，每通道独占）
  auto *pch = new tcp_buf_ch();
  my_recv_cb rcb;
  my_ch_op cop;

  int32 ret = pch->connect_ch(battr, cattr, &rcb, &cop,
                              &recv_th, &remote, /*local*/nullptr,
                              /*asyn*/0);
  if (ret < 0) {
    lb_comm_err err; perror_lb(err, ret);
    fprintf(stderr, "connect_ch 失败: %s\n", err.err_msg);
    delete pch; recv_th.to_stop(); recv_th.join();
    return 1;
  }

  // 6) 启动接收（把 aio_tcp 加入 mthread 的 epoll）
  ret = pch->start_ch();
  if (ret < 0) {
    lb_comm_err err; perror_lb(err, ret);
    fprintf(stderr, "start_ch 失败: %s\n", err.err_msg);
    delete pch; recv_th.to_stop(); recv_th.join();
    return 1;
  }

  // 7) 主循环：epoll 线程在后台跑，deal_msg 在 recv_th 上回调
  //    主线程等连接关闭
  while (!rcb.got_reply && pch->is_work()) {
    comm_utils::sleep_ms(50);  // 见 mutils.h
  }

  // 8) 清理
  if (pch->is_work()) pch->close_ch(0);
  // 等 deal_ch_closed 在 recv_th 上触发后，delete 才安全
  while (!pch->is_free()) {
    comm_utils::sleep_ms(10);
  }
  delete pch;
  recv_th.to_stop();
  recv_th.join();
  return 0;
}
```

## 4. 关键 API 形态速记

| 调用 | 作用 | 返回 |
|------|------|------|
| `mthread::init_th(id,maxpoll,eventq_size,eventq_loop,waitms)` | 配置线程 | void |
| `mthread::run()` | 启动 pthread | void |
| `aio_tcp::connect_ch(battr,cattr,rcb,cop,th,&remote,...)` | 客户端建链 | `0=成功 / <0=失败` |
| `aio_tcp::start_ch()` | 把通道加入 th 的 epoll | `0=成功 / <0=失败` |
| `aio_tcp::send_msg(buf,len)` | 发送（**仅 recv_th 同线程安全**）| `>0=已发字节数 / <0=错误` |
| `aio_tcp::close_ch(err=0)` | 关闭（首次成功只调一次 deal_ch_closing）| void |
| `aio_tcp::is_work() / is_close() / is_free()` | 状态查询 | bool |

## 5. 配套 echo 服务器（test 环境用 netcat）

```bash
# 在另一个终端
nc -l 9000 <<< "echo: hello"
```

## 6. 编译运行

```bash
cd my_app/build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
./my_app
# 期望输出：
# 已连接，开始发 hello
# 收到 7 字节: echo: hello
# 连接已关闭 (err=0)
```

## 7. 常见第一坑

| 现象 | 原因 | 解决 |
|------|------|------|
| `connect_ch` 返回 `LBERR_CH_CONNECT_FAIL(-34)` | `remote` 未正确 set，或端口不可达 | `comm_sock.h` 的 `csock_addr::set_addr(ip, port)` 用法；先 nc 验证端口 |
| `start_ch` 返回 `LBERR_OBJ_NOT_HAVE` | `th_recv == NULL` | `connect_ch` 前确保 `mthread::run()` 已调 |
| `deal_msg` 不触发 | `start_ch` 没调，或 mthread 已退出 | 确认 epoll 路径已挂上 |
| `send_msg` 返回 `< 0`，对端收到乱序 | 在非 recv_th 线程调 `send_msg` | 把发送封成 `event_op`，投递到 recv_th（见 [05 §1.4](./05-business-recipes.md#14-发送串行化event_op-投递)） |
| 进程退出后资源未释放 | 没等 `is_free()` 就 `delete` | 主循环查 `is_free()` 再 delete |
| 多个客户端发消息时数据错乱 | `send_msg` 多线程并发 | 必须串行化（4 种模式见 [网络.md §发送模式](../使用范式/网络.md#发送模式与线程安全)） |

## 8. 下一步

- 多连接/多线程服务器：[05-business-recipes.md §1](./05-business-recipes.md#1-网络服务器组装)
- 跨进程数据共享：[05-business-recipes.md §2](./05-business-recipes.md#2-跨进程共享组装)
- 心跳与重连：[05-business-recipes.md §3](./05-business-recipes.md#3-重连心跳组装)
- API 完整签名：直接读 `../common/include/aio_tcp.h`、`mthread.h`、`comm_sock.h`
