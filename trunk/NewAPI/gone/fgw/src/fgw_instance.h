// fgw_instance - fgw 服务实例（编排入口）
//
// Step 5 已确认成员定义
// S-0：系统视角
//   - 1 句话：fgw 是 api 客户端与 fdm 后台之间的 FPGA 柜台网关，
//            负责多板卡路由 + 多客户接入 + 投资者登录跨链接中转 + 证券同步
//   - 边界：包含 api 监听 / fdm 连接 / 路由 / 数据加载；不含策略 / 行情
// R-11：系统流程完整性（启动 / 关闭 / 周期 / 信号 / 错误恢复 全覆盖）
//   - init / start / stop / join：完整生命周期
//   - periodic_check：周期检查（main_thread 驱动）
//
// 接口（Step 5 已确认业务操作，Step 6-8 细化）：
//   - init(cfg_file)：读配置 + 初始化子系统
//   - start()       ：启动后台任务 + 监听 api
//   - stop()        ：停止接收 + 关闭所有链接
//   - join()        ：等待所有线程结束
//   - periodic_check()：周期检查（fdm 重连 + 心跳超时）

#pragma once

#include "data_engine.h"
#include "fdm_engine.h"

#include "api_engine.h"
#include "fgw_config.h"
#include "mlog.h"

namespace lb_fgw {

/// fgw 服务实例（编排入口）
class fgw_instance {
public:
  // 业务操作
  int init(const char *cfg_file);
  int start();
  void stop();

  // 周期性检查（R-11：fdm 重连 + 心跳超时）
  void periodic_check();

  // 析构：3 引擎分别析构 send_ques（内部拥有）

protected:
  // todo : 若需要的内部公共函数

  /// 读配置（fgw_instance::init 调用）
  int load_config(fgw_config &o_cfg, const char *cfg_file);

private:
  // 3 个引擎（fgw 编排的核心子系统）
  api_engine api_eng_;
  data_engine data_eng_; // 保留原文拼写（成员名带下划线后缀）
  fdm_engine fdm_eng_;

  lb_common::lb_log log_; ///< 日志
};

} // namespace lb_fgw