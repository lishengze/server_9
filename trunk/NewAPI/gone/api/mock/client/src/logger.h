// logger.h - 测试程序日志系统
//
// 职责：为 mock_client 提供完整的分级日志系统。
//   - 日志分级：DEBUG / INFO / WARN / ERROR / FATAL 五级
//   - 输出目标：同时输出到屏幕（控制台）和日志文件
//   - 线程安全：内部使用互斥锁，支持多线程并发写日志
//   - 格式：[时间戳] [级别] [文件:行] 消息
//
// 使用方式：
//   1. 程序启动时调用 Logger::instance().init(log_file, min_level)
//   2. 任意位置使用 LOG_DEBUG / LOG_INFO / LOG_WARN / LOG_ERROR / LOG_FATAL
//      宏记录日志（支持 << 流式拼接）

#ifndef MOCK_CLIENT_LOGGER_H
#define MOCK_CLIENT_LOGGER_H

#include <string>
#include <mutex>
#include <fstream>
#include <sstream>

namespace mock {

/// 日志级别
enum class LogLevel {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
    FATAL = 4
};

/// 级别转字符串
const char* level_str(LogLevel level);

/// 日志器（单例）
class Logger {
public:
    /// 获取全局单例
    static Logger& instance();

    /// 初始化：设置日志文件路径、最低输出级别、是否输出到控制台
    /// @param log_file        日志文件路径（空则不写文件）
    /// @param min_level       最低日志级别（低于该级别的日志被丢弃）
    /// @param console_enabled 是否输出到屏幕（默认 true）
    void init(const std::string& log_file, LogLevel min_level, bool console_enabled = true);

    /// 动态调整日志级别
    void set_level(LogLevel level);

    /// 获取当前最低级别
    LogLevel level() const { return min_level_; }

    /// 获取日志文件路径
    const std::string& log_file() const { return log_file_; }

    /// 记录一条日志
    /// @param level 日志级别
    /// @param file  源文件名（__FILE__）
    /// @param line  源行号（__LINE__）
    /// @param msg   日志消息
    void log(LogLevel level, const char* file, int line, const std::string& msg);

    /// 关闭日志文件（flush 并关闭）
    void close();

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex mutex_;          ///< 保护文件与控制台输出
    std::ofstream file_;        ///< 日志文件
    LogLevel min_level_;        ///< 最低输出级别
    bool console_enabled_;      ///< 是否输出到控制台
    std::string log_file_;      ///< 日志文件路径
};

} // namespace mock

// ========== 便捷宏（支持 << 流式拼接，兼容 gcc 4.8.5） ==========
#define LOG_DEBUG(msg) do { \
    std::ostringstream LOG_oss_; \
    LOG_oss_ << msg; \
    ::mock::Logger::instance().log(::mock::LogLevel::DEBUG, __FILE__, __LINE__, LOG_oss_.str()); \
} while(0)
#define LOG_INFO(msg) do { \
    std::ostringstream LOG_oss_; \
    LOG_oss_ << msg; \
    ::mock::Logger::instance().log(::mock::LogLevel::INFO, __FILE__, __LINE__, LOG_oss_.str()); \
} while(0)
#define LOG_WARN(msg) do { \
    std::ostringstream LOG_oss_; \
    LOG_oss_ << msg; \
    ::mock::Logger::instance().log(::mock::LogLevel::WARN, __FILE__, __LINE__, LOG_oss_.str()); \
} while(0)
#define LOG_ERROR(msg) do { \
    std::ostringstream LOG_oss_; \
    LOG_oss_ << msg; \
    ::mock::Logger::instance().log(::mock::LogLevel::ERROR, __FILE__, __LINE__, LOG_oss_.str()); \
} while(0)
#define LOG_FATAL(msg) do { \
    std::ostringstream LOG_oss_; \
    LOG_oss_ << msg; \
    ::mock::Logger::instance().log(::mock::LogLevel::FATAL, __FILE__, __LINE__, LOG_oss_.str()); \
} while(0)

#endif // MOCK_CLIENT_LOGGER_H