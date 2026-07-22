#pragma once

/**
 * @file mlog.h
 * @brief 日志模块
 *
 * 提供异步/同步日志记录功能。核心类 lb_log 继承自 simple_thread，
 * 支持两种工作模式：
 * - 同步模式：队列大小 = 0 时，write_log 直接写文件
 * - 异步模式：队列大小 > 0 时，启动后台线程消费队列并写文件
 *
 * lb_log_hand 是单线程日志句柄，提供流式 API（<< 运算符）和宏封装，
 * 每个线程使用独立的句柄实例，内部不做锁保护。
 *
 * 使用示例：
 * @code
 *   lb_log glog;
 *   glog.open_log("./log/", "app", 1, 20240101, 10*1024*1024);
 *
 *   lb_log_hand tlh(&glog);
 *   info_log(tlh) << "server started, port=" << 8080 << end_log;
 *   error_log(tlh) << "connection failed, fd=" << fd << end_log;
 * @endcode
 *
 * 日志级别（数值越小越详细）：
 * - 0: DEBUG - 调试信息
 * - 1: INFO  - 一般信息
 * - 2: WARN  - 警告信息
 * - 3: ERROR - 错误信息
 * - 4: FATAL - 致命错误
 */

#include "comm_sys.h"
#include "mlock.h"
#include "que_mth_buf.h"
#include "simple_thread.h"

#include <stdio.h>

namespace lb_common {

/** @brief 日志消息最大长度（含结束符） */
#define LBLOG_MSG_MAX_LEN 512
/** @brief 日志消息可用长度（不含前缀预留空间） */
#define LBLOG_MSG_AVAIL_LEN 500
/** @brief 日志文件路径最大长度 */
#define LBLOG_FILE_PATH_LEN 256

/**
 * @brief 日志处理类的前向声明
 *
 * lb_log_hand 是单线程日志句柄，在不知道类完整定义的情况下通过前向声明引用。
 */
class lb_log_hand;

/**
 * @brief 日志类
 *
 * 继承自 simple_thread，支持异步/同步两种日志记录模式。
 * 异步模式下启动后台线程从队列中消费日志消息并写入文件。
 * 同步模式下直接在当前调用线程中写入文件。
 */
class lb_log : public simple_thread {
protected:
  /** @brief 默认日志文件存放路径 */
#define LOG_DEFAULT_PATH "./run_log/"
  /** @brief 单个日志文件最大行数，超过后自动切换新文件 */
#define LOG_FILE_LINE_MAXNUM 2000000

  /**
   * @brief 日志队列头部结构体
   *
   * 每条日志消息在队列中的固定头部，记录消息的长度和级别。
   */
  struct log_que_head {
    int32 alen;   ///< 日志内容长度（不含头部）
    int32 clevel; ///< 日志级别
  };

  que_mth_buf *pq;                    ///< 日志队列缓冲区指针（NULL=同步模式）
  int32 level;                        ///< 当前日志输出级别，只输出 <= level 的消息
  int32 date;                         ///< 当前日期（YYYYMMDD 格式），用于日志文件按日切换
  cmutex filelock;                    ///< 文件操作互斥锁，保护 fp 的并发写入
  int32 fileno;                       ///< 当前文件编号，同一天可产生多个文件
  int32 linenum;                      ///< 当前文件已写入行数
  FILE *fp;                           ///< 当前打开的日志文件指针
  char pathname[LBLOG_FILE_PATH_LEN]; ///< 日志文件基础路径名

  /**
   * @brief 将整数转换为字符串写入缓冲区
   * @param[out] o_buf 输出缓冲区
   * @param[in] num 待转换的整数
   * @return 写入的字符数
   */
  static int32 int_str(char *o_buf, int64 num);

  /**
   * @brief 格式化日志级别为可读字符串
   * @param[out] o_buf 输出缓冲区
   * @param[in] wlevel 日志级别（0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL）
   * @return 写入的字符数
   */
  int32 format_level(char *o_buf, int32 wlevel);

  /**
   * @brief 日志预处理：在消息前添加时间戳、文件名、行号、日志级别等前缀
   * @param[out] o_buf 输出缓冲区
   * @param[in] file 源文件名（__FILE__ 宏）
   * @param[in] line 源文件行号（__LINE__ 宏）
   * @param[in] wlevel 日志级别
   * @return 前缀总长度
   */
  int32 log_pre(char *o_buf, const char *file, int32 line, int32 wlevel);

  /**
   * @brief 将已格式化的日志消息写入当前文件
   * @param[in] pmsg 已格式化完成的日志消息字符串
   */
  void write_file(const char *pmsg);

  /**
   * @brief 后台线程工作函数（simple_thread 接口）
   *
   * 循环从队列中取日志消息，调用 write_file 写入文件。
   */
  virtual void do_work();

  /**
   * @brief 判断是否需要处理日志（simple_thread 接口）
   * @return TRUE=队列中有待处理消息，需要工作
   */
  virtual BOOL need_work() { return (pq->get_used() > 0); }

  /**
   * @brief 销毁日志资源
   *
   * 关闭日志文件，释放队列缓冲区，停止后台线程。
   */
  void destroy();

public:
  /** @brief lb_log_hand 可访问本类的私有和保护成员 */
  friend class lb_log_hand;

  /**
   * @brief 获取当前日志输出级别
   * @return 日志级别值
   */
  FORCE_INLINE int32 get_level() { return level; }

  /**
   * @brief 打开并初始化日志系统
   *
   * @param[in] path 日志文件存放目录路径
   * @param[in] tprename 日志文件名前缀
   * @param[in] loglevel 日志输出级别，只输出 <= loglevel 的消息
   * @param[in] curdate 当前日期（YYYYMMDD）
   * @param[in] quesize 队列大小（字节），0=同步模式，>0=异步模式
   * @param[in] cpuid 后台线程绑定的 CPU ID，-1 表示不绑定
   * @return 0=成功，<0=失败
   */
  int32 open_log(const char *path, const char *tprename, int32 loglevel, int32 curdate, int32 quesize = 0,
                 int32 cpuid = -1);

  /**
   * @brief 切换日志文件日期
   *
   * 当日期变化时调用，关闭旧文件，创建新日期命名的日志文件。
   *
   * @param[in] newdate 新日期（YYYYMMDD）
   * @return 0=成功，<0=失败
   */
  int32 change_date(int32 newdate);

  /**
   * @brief 写入一条日志消息
   *
   * 同步模式直接调用 write_file 写入文件；
   * 异步模式将消息放入队列，由后台线程消费。
   *
   * @param[in] pmsg 日志消息内容（应以 '\0' 结尾）
   * @param[in] msglen 消息长度（含结束符）
   * @param[in] msglevel 日志级别
   */
  void write_log(const char *pmsg, int32 msglen, int32 msglevel);

  /**
   * @brief 关闭日志系统
   *
   * 停止后台线程，刷新并关闭文件，释放队列资源。
   */
  void close_log();

  /** @brief 构造函数，初始化所有成员为零/NULL */
  lb_log() : pq(NULL), fileno(0), linenum(0), fp(NULL){};

  /** @brief 析构函数，调用 destroy() 释放资源 */
  virtual ~lb_log() { destroy(); }
};

/**
 * @brief 单线程日志句柄
 *
 * 每个线程应使用独立的 lb_log_hand 实例。
 * 提供流式日志 API（通过 << 运算符和 end_log 宏），
 * 内部维护一个 512 字节的缓冲区，积累日志内容后整体提交到 lb_log。
 *
 * 使用示例：
 * @code
 *   lb_log_hand tlh(&glog);
 *   error_log(tlh) << "code=" << code << ", price=" << price << end_log;
 * @endcode
 */
class lb_log_hand {
private:
  int32 curlevel;              ///< 当前日志消息的级别
  int32 wrlevel;               ///< 日志输出阈值级别（来自 lb_log::level）
  int32 curlen;                ///< 当前缓冲区已使用长度
  lb_log *mlog;                ///< 关联的 lb_log 实例指针
  char buf[LBLOG_MSG_MAX_LEN]; ///< 日志消息组装缓冲区

  /**
   * @brief 判断当前消息级别是否被屏蔽
   * @return TRUE=消息级别高于输出阈值，不输出
   */
  FORCE_INLINE BOOL no_log() { return wrlevel > curlevel; }

  /**
   * @brief 将整数转换为十进制字符串写入缓冲区
   *
   * 仅当缓冲区剩余空间足够时才写入，防止溢出。
   *
   * @tparam T 整数类型（int8~int64, uint8~uint64）
   * @param[in] num 待转换的整数
   */
  template <typename T> void itostr(T num) {
    const char *preval = "0123456789";
    char tstr[64];
    uint64 tv;
    int32 tlen = 0;
    if (num >= 0) {
      tv = num;
    } else {
      tv = -num;
    }
    do {
      uint64 td = tv / 10;
      tstr[tlen++] = preval[tv - td * 10];
      tv = td;
    } while (tv);

    if (tlen + curlen <= LBLOG_MSG_AVAIL_LEN) {
      if (num < 0) {
        buf[curlen++] = '-';
      }
      for (int32 i = tlen - 1; i >= 0; i--) {
        buf[curlen++] = tstr[i];
      }
    }
  }

  /**
   * @brief 将整数转换为大写十六进制字符串写入缓冲区
   *
   * 仅当缓冲区剩余空间足够时才写入。
   *
   * @tparam T 整数类型
   * @param[in] num 待转换的整数
   */
  template <typename T> void itostrX(T num) {
    const char *preval = "0123456789ABCDEF";
    char tstr[64];
    uint64 tv;
    int32 tlen = 0;
    if (num >= 0) {
      tv = num;
    } else {
      tv = -num;
    }
    do {
      tstr[tlen++] = preval[tv & 15];
      tv = (tv >> 4);

    } while (tv);

    if (tlen + curlen <= LBLOG_MSG_AVAIL_LEN) {
      if (num < 0) {
        buf[curlen++] = '-';
      }
      for (int32 i = tlen - 1; i >= 0; i--) {
        buf[curlen++] = tstr[i];
      }
    }
  }

  /**
   * @brief 判断字符是否为可安全打印的字符
   *
   * 可安全打印的字符包括：数字、字母、常见标点符号。
   * 不安全字符（如控制字符）将转为数字形式输出。
   *
   * @param[in] val 待判断的字符
   * @return TRUE=可安全打印，FALSE=需要转为数字形式
   */
  FORCE_INLINE BOOL is_support(char val) {
    if (val == ',' || val == '=' || val == ';' || val == ':' || (val >= '0' && val <= '9') ||
        (val >= 'a' && val <= 'z') || (val >= 'A' && val <= 'Z') || val == ' ' || val == '[' || val == ']' ||
        val == '(' || val == ')' || val == '{' || val == '}') {
      return TRUE;
    }
    return FALSE;
  }

public:
  /**
   * @brief 开始一条指定级别的日志消息
   *
   * 检查日志级别过滤，设置当前级别，准备开始填充日志内容。
   *
   * @param[in] loglevel 日志级别
   * @return 自身的引用，用于链式调用
   */
  lb_log_hand &log_begin(int32 loglevel);

  /**
   * @brief 开始一条带有位置信息的日志消息
   *
   * 若日志级别未屏蔽，写入时间戳、文件名、行号等前缀。
   *
   * @param[in] file 源文件名（__FILE__）
   * @param[in] line 源文件行号（__LINE__）
   * @param[in] loglevel 日志级别
   * @return 自身的引用
   */
  lb_log_hand &log_begin(const char *file, int32 line, int32 loglevel);

  /**
   * @brief 结束并提交当前日志消息
   *
   * 在消息末尾添加 '\0' 结束符，按 4 字节对齐后提交到 lb_log::write_log。
   * 如果缓冲区为空则不提交。
   */
  FORCE_INLINE void log_end() {
    if (curlen == 0)
      return;
    buf[curlen++] = '\0';
    if ((curlen & 3) != 0) {
      buf[curlen] = '\0';
      curlen += (curlen & 3);
    }
    mlog->write_log(buf, curlen, curlevel);
    curlen = 0;
  }

  /**
   * @brief 输出字符串到日志缓冲区
   *
   * 若 val 为 NULL，则直接调用 log_end() 结束并提交日志消息。
   * （这是 end_log 宏的实现原理：end_log 定义为 (char*)NULL）
   *
   * @param[in] val 要输出的字符串指针，NULL 表示结束提交
   */
  inline void log_str(const char *val) {
    if (NULL != val) {
      if (no_log())
        return;

      const char *pc = val;
      while ((curlen < LBLOG_MSG_AVAIL_LEN) && (*pc != '\0')) {
        buf[curlen++] = *pc++;
      }
    } else {
      log_end();
    }
  }

  /**
   * @brief 输出 int64 整数到日志缓冲区
   * @param[in] val 整数值
   */
  FORCE_INLINE void log_int(int64 val) {
    if (no_log())
      return;
    itostr<int64>(val);
  }

  /** @copydoc log_int(int64) */
  FORCE_INLINE void log_int(uint64 val) {
    if (no_log())
      return;
    itostr<uint64>(val);
  }

  /** @copydoc log_int(int64) */
  FORCE_INLINE void log_int(int32 val) {
    if (no_log())
      return;
    itostr<int32>(val);
  }

  /** @copydoc log_int(int64) */
  FORCE_INLINE void log_int(uint32 val) {
    if (no_log())
      return;
    itostr<uint32>(val);
  }

  /** @copydoc log_int(int64) */
  FORCE_INLINE void log_int(int16 val) {
    if (no_log())
      return;
    itostr<int16>(val);
  }

  /** @copydoc log_int(int64) */
  FORCE_INLINE void log_int(uint16 val) {
    if (no_log())
      return;
    itostr<uint16>(val);
  }

  /** @copydoc log_int(int64) */
  FORCE_INLINE void log_int(int8 val) {
    if (no_log())
      return;
    itostr<int8>(val);
  }

  /** @copydoc log_int(int64) */
  FORCE_INLINE void log_int(uint8 val) {
    if (no_log())
      return;
    itostr<uint8>(val);
  }

  /**
   * @brief 输出单个字符到日志缓冲区
   *
   * 空字符 '\0' 不会写入（它被用于结束日志消息）。
   *
   * @param[in] val 字符值
   */
  FORCE_INLINE void log_char(char val) {
    if (no_log())
      return;
    if (val != '\0')
      buf[curlen++] = val;
  }

  /** @copydoc log_char(char) */
  FORCE_INLINE void log_char(unsigned char val) {
    if (no_log())
      return;
    if (val != '\0')
      buf[curlen++] = val;
  }

  // --- 流式运算符重载 ---

  /** @brief 流式输出字符串 */
  lb_log_hand &operator<<(const char *val) {
    log_str(val);
    return *this;
  }

  /** @brief 流式输出 int64 */
  lb_log_hand &operator<<(int64 val) {
    log_int(val);
    return *this;
  }

  /** @brief 流式输出 uint64 */
  lb_log_hand &operator<<(uint64 val) {
    log_int(val);
    return *this;
  }

  /** @brief 流式输出 int32 */
  lb_log_hand &operator<<(int32 val) {
    log_int(val);
    return *this;
  }

  /** @brief 流式输出 uint32 */
  lb_log_hand &operator<<(uint32 val) {
    log_int(val);
    return *this;
  }

  /** @brief 流式输出 int16 */
  lb_log_hand &operator<<(int16 val) {
    log_int(val);
    return *this;
  }

  /** @brief 流式输出 uint16 */
  lb_log_hand &operator<<(uint16 val) {
    log_int(val);
    return *this;
  }

  /**
   * @brief 流式输出 char
   *
   * 若字符不是可安全打印字符（控制字符等），则转为整数形式输出。
   * 若字符是可安全打印字符，直接输出该字符。
   */
  lb_log_hand &operator<<(char val) {
    if (is_support(val)) {
      log_char(val);
    } else {
      log_int((int8)val);
    }
    return *this;
  }

  /**
   * @brief 流式输出 unsigned char
   *
   * 与 char 版本同样的安全打印判断逻辑。
   */
  lb_log_hand &operator<<(unsigned char val) {
    if (is_support(val)) {
      log_char(val);
    } else {
      log_int((uint8)val);
    }
    return *this;
  }

  /**
   * @brief 初始化日志句柄
   *
   * 关联到指定的 lb_log 实例并设置日志输出阈值。
   *
   * @param[in] plog 关联的 lb_log 实例指针
   * @param[in] mylevel 本句柄的日志输出阈值，0 表示使用 lb_log 的全局级别
   */
  void init(lb_log *plog, int32 mylevel = 0);

  /** @brief 默认构造函数，初始化为空状态 */
  lb_log_hand() {
    curlevel = 0;
    wrlevel = 0;
    curlen = 0;
    mlog = NULL;
  }

  /**
   * @brief 带 lb_log 的构造函数
   * @param[in] plog 关联的 lb_log 实例
   * @param[in] mylevel 本句柄的日志输出阈值
   */
  lb_log_hand(lb_log *plog, int32 mylevel = 0) { init(plog, mylevel); }

  /**
   * @brief 带位置信息的构造函数
   *
   * 构造后立即开始一条带文件名和行号的日志消息。
   *
   * @param[in] plog 关联的 lb_log 实例
   * @param[in] file 源文件名
   * @param[in] line 源文件行号
   * @param[in] mylevel 日志级别
   */
  lb_log_hand(lb_log *plog, const char *file, int32 line, int32 mylevel) {
    assert(NULL != plog);
    wrlevel = plog->get_level();
    curlen = 0;
    mlog = plog;
    log_begin(file, line, mylevel);
  }

  /** @brief 析构函数 */
  ~lb_log_hand(){};
};

/**
 * @brief 日志结束标记宏
 *
 * 传入 log_str(NULL) 触发 log_end()，提交并写入当前日志消息。
 * 通常放在一条流式日志语句的最末尾。
 *
 * 使用示例：
 * @code
 *   info_log(tlh) << "key=" << key << ", val=" << val << end_log;
 * @endcode
 */
#define end_log ((char *)NULL)

/**
 * @brief DEBUG 级别日志开始宏
 *
 * 开始一条 DEBUG 级别的日志消息，自动附加 __FILE__ 和 __LINE__。
 * @param[in] log_hand lb_log_hand 实例
 */
#define debug_log(log_hand) (log_hand).log_begin(__FILE__, __LINE__, 0)

/**
 * @brief INFO 级别日志开始宏
 *
 * 开始一条 INFO 级别的日志消息，自动附加 __FILE__ 和 __LINE__。
 * @param[in] log_hand lb_log_hand 实例
 */
#define info_log(log_hand) (log_hand).log_begin(__FILE__, __LINE__, 1)

/**
 * @brief WARNING 级别日志开始宏
 *
 * 开始一条 WARNING 级别的日志消息，自动附加 __FILE__ 和 __LINE__。
 * @param[in] log_hand lb_log_hand 实例
 */
#define warning_log(log_hand) (log_hand).log_begin(__FILE__, __LINE__, 2)

/**
 * @brief ERROR 级别日志开始宏
 *
 * 开始一条 ERROR 级别的日志消息，自动附加 __FILE__ 和 __LINE__。
 * @param[in] log_hand lb_log_hand 实例
 */
#define error_log(log_hand) (log_hand).log_begin(__FILE__, __LINE__, 3)

/**
 * @brief FATAL 级别日志开始宏
 *
 * 开始一条 FATAL 级别的日志消息，自动附加 __FILE__ 和 __LINE__。
 * @param[in] log_hand lb_log_hand 实例
 */
#define fatal_log(log_hand) (log_hand).log_begin(__FILE__, __LINE__, 4)

} // namespace lb_common
