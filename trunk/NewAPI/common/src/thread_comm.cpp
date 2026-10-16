
#include "thread_comm.h"
#include "comm_errno.h"

#include <errno.h>
#include <signal.h>

namespace lb_common
{

  volatile sig_atomic_t g_signal_stopctl = -1;

  typedef void (*f_signal_hand)(int signal_no);

  int32 f_set_signal_hand(int32 signal_no, f_signal_hand func)
  {
    // L1: SIGKILL/SIGSTOP 不可捕获/忽略，sigaction 必然失败。
    // 原代码 return 0 让上层误以为注册成功。改为返回错误码。
    if (signal_no == SIGKILL || signal_no == SIGSTOP)
      return LBERR_ARGV_WRONG;

    struct sigaction sig_act, old_act;
    memset((void *)&sig_act, 0, sizeof(sig_act));
    sig_act.sa_handler = func;
    sigemptyset(&sig_act.sa_mask);
    if (signal_no != SIGALRM)
      sigaddset(&sig_act.sa_mask, SIGALRM);

    sig_act.sa_flags = 0;
#ifdef SA_INTERRUPT
    sig_act.sa_flags |= SA_INTERRUPT;
#endif

    int32 ret = 0;
    while ((ret = sigaction(signal_no, &sig_act, &old_act)) < 0)
    {
      if (errno != EINTR)
        return LBERR_ATTR_SET_FAIL;
    }

    return 0;
  }

  void f_deal_signal_stop(int32 signal_no) { g_signal_stopctl = signal_no; }

  void install_signal_hand()
  {
    f_set_signal_hand(SIGTTOU, SIG_IGN);
    f_set_signal_hand(SIGTTIN, SIG_IGN);
    f_set_signal_hand(SIGTSTP, SIG_IGN);
    f_set_signal_hand(SIGPIPE, SIG_IGN);
    f_set_signal_hand(SIGHUP, SIG_IGN);
    f_set_signal_hand(SIGABRT, f_deal_signal_stop);
    f_set_signal_hand(SIGBUS, f_deal_signal_stop);
    // f_set_signal_hand 内部已跳过，这里删除无效调用避免语义混乱
    f_set_signal_hand(SIGQUIT, f_deal_signal_stop);
    f_set_signal_hand(SIGTERM, f_deal_signal_stop);
  }

  void mask_thread_signal()
  {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGTTOU);
    sigaddset(&set, SIGTTIN);
    sigaddset(&set, SIGTSTP);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
  }

  int32 run_thread(pthread_t &o_thid, f_thread_comfunc pfunc, void *th_arg)
  {
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0)
    {
      return LBERR_ATTR_INIT_FAIL;
    }

    int32 ret = 0;
    if (::pthread_create(&o_thid, &attr, pfunc, th_arg) != 0)
    {
      ret = LBERR_OBJ_OPEN_FAIL;
    }

    pthread_attr_destroy(&attr);

    return ret;
  }

  void wait_thread(pthread_t th_id)
  {
    if (th_id > 0)
      pthread_join(th_id, NULL);
  }

} // namespace lb_common
