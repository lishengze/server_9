// fgw - FPGA 柜台网关服务入口

#include <csignal>
#include <cstdlib>

#include "fgw_instance.h"
#include "thread_comm.h"

int main(int argc, char *argv[]) {

  const char *def_cfgfile = "./fgw_cfg.ini";

  // common 库信号处理
  lb_common::install_signal_hand();

  lb_fgw::fgw_instance *g_fgw = new lb_fgw::fgw_instance;
  int ret = g_fgw->init(def_cfgfile);
  if (ret < 0) {
    return ret;
  }

  ret = g_fgw->start();
  if (ret < 0) {
    return ret;
  }

  while (lb_common::g_signal_stopctl == -1) {
    g_fgw->periodic_check();
  }

  g_fgw->stop();

  delete g_fgw;
  return 0;
}
