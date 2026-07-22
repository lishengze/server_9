
#pragma once

// link_send_que - 链接发送队列 + 触发 fd（每线程一个，多写一读）
//
// Step 5 已确认成员定义；Step 6 摸底后模板化为 link_send_que<LINK>
//   - 模板化理由 [I]：骨架 link_event_qhead<LINK> 已是模板（link_ptr_ 是 LINK*），
//     deal_event 须按 LINK 类型调用 link->send_msg/close_link，故 link_send_que
//     必须与 link_event_qhead 同构为模板。持有关系/职责不变，符合 common 库
//     aio_tcp/hash_map_mth 全头文件内联模板的惯例。
//
// 串行化语义（common-usage 网络.md §发送模式）：
//   - send_msg 非线程安全 → 业务线程把"发送事件"入队（push_send_msg），
//     发送线程单线程消费 deal_event → 调 link->send_msg，天然串行
//   - link_send_que 继承 epoll_event_op，挂载到发送线程的 mthread epoll，
//     waker_(eventfd) 触发 deal_event
//
// 接口（Step 8 S1 增量）：
//   - init(que_size)                 ：初始化队列 + waker
//   - push_send_msg(link, msg, len)  ：投递"发送消息"事件（多线程安全）
//   - push_close_link(link)          ：投递"关闭链接"事件（多线程安全）
//   - deal_event()                   ：发送线程消费队列，调 link->send_msg/close_link

#include <cstdint>
#include <cstring>

#include "comm_sys.h"
#include "fgw_errno.h"
#include "que_mth_buf.h"
#include "wait_poll.h"
#include "wait_wake.h"

namespace lb_fgw {

//发送队列中事件类型
static constexpr int link_event_send_msg = 0;   // 发送消息
static constexpr int link_event_close_link = 1; // 关闭链接
static constexpr int link_event_discard = 2;    // 事件废弃

//发送队列中事件头
template <class LINK> struct link_event_qhead {
  int32_t type_;
  int32_t len_; // 不含头
  LINK *link_ptr_;
};

/// 链接发送队列和触发fd（模板化：deal_event 按 LINK 类型调用 link 方法）
/// 数量级：每个线程一个
/// 生命周期：引擎中，全局
template <class LINK, class ENG> class link_send_que final : public lb_common::epoll_event_op {
public:
  /// 初始化发送队列 + 触发 fd
  int32_t init(ENG *peng, int64_t que_size, int32_t once_send_num) {
    once_send_num_ = once_send_num;
    wait_flag_ = 1;
    int32_t ret = send_que_.init(que_size);
    if (ret < 0)
      return ret;
    ret = waker_.init(1);
    if (ret < 0)
      return ret;
    set_event(0, 0, 0); // 输入事件，水平触发
    link_eng_ = peng;
    return FGW_OK;
  }

  /// 投递"发送消息"事件（多线程安全）
  /// @return FGW_OK 成功；FGW_ERR_BUF_FULL 队列满
  int32_t push_send_msg(LINK *link, const char *msg, int32_t len) {
    if (unlikely(!link->is_work()))
      return FGW_ERR_LINK_STATE;
    int32_t total = (int32_t)sizeof(link_event_qhead<LINK>) + len;
    char *pdata = nullptr;
    int64_t pos = send_que_.write_get_mth(pdata, total);
    if (pos == 0)
      return FGW_ERR_BUF_FULL;
    link_event_qhead<LINK> *head = reinterpret_cast<link_event_qhead<LINK> *>(pdata);
    head->type_ = link_event_send_msg;
    head->len_ = len;
    head->link_ptr_ = link;
    if (len > 0)
      std::memcpy(pdata + sizeof(link_event_qhead<LINK>), msg, len);

    if (likely(link->is_work())) {
      send_que_.write_cmt_mth(pos, total);
      if (wait_flag_)
        waker_.wake();
      return FGW_OK;
    } else {
      head->type_ = link_event_discard;
      send_que_.write_cmt_mth(pos, total);
      if (wait_flag_)
        waker_.wake();
      return FGW_ERR_LINK_STATE;
    }
  }

  /// 投递"关闭链接"事件（多线程安全）
  /// @return FGW_OK 成功；FGW_ERR_BUF_FULL 队列满
  int32_t push_close_link(LINK *link) {
    int32_t total = (int32_t)sizeof(link_event_qhead<LINK>);
    char *pdata = nullptr;
    int64_t pos = 0;
    while ((pos = send_que_.write_get_mth(pdata, total)) <= 0)
      ;
    link_event_qhead<LINK> *head = reinterpret_cast<link_event_qhead<LINK> *>(pdata);
    head->type_ = link_event_close_link;
    head->len_ = 0;
    head->link_ptr_ = link;
    send_que_.write_cmt_mth(pos, total);
    if (wait_flag_)
      waker_.wake();
    return FGW_OK;
  }

protected:
  /// 发送线程主处理：清 waker → 循环读队列 → 按 type 调 link 方法
  void deal_event() override {
    wait_flag_ = 0;
    int32_t i = once_send_num_;
    char *pdata = nullptr;
    int64_t pos;
    while (i > 0) {
      pos = send_que_.read_get(pdata);
      if (pos <= 0) {
        wait_flag_ = 1;
        waker_.reset();
        return;
      }

      link_event_qhead<LINK> *head = reinterpret_cast<link_event_qhead<LINK> *>(pdata);
      int32_t total = (int32_t)sizeof(link_event_qhead<LINK>) + head->len_;
      if (head->type_ == link_event_send_msg) {
        char *msg = pdata + sizeof(link_event_qhead<LINK>);
        int32_t ret = head->link_ptr_->send_msg(msg, head->len_);
        if (ret < 0) {
          link_eng_->deal_send_error(head->link_ptr_, msg, head->len_, ret);
        }
      } else if (head->type_ == link_event_close_link) {
        // close_ch 设置 CLOSING（外部可能已调，幂等）+ sub_ref 释放发送线程引用
        head->link_ptr_->deal_close_event();
      }
      send_que_.read_cmt(total);
    }

    //wait_flag_ = 1;
  }

  void deal_error() override {}
  void deal_close() override {}
  int32_t get_fd() override { return waker_.get_fd(); }

private:
  int32_t once_send_num_;
  int32_t wait_flag_;
  lb_common::que_mth_buf send_que_; ///< 发送队列（多写一读）
  lb_common::event_wake waker_;     ///< 入队触发
  ENG *link_eng_;
};

} // namespace lb_fgw
