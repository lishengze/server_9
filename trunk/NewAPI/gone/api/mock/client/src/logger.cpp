// logger.cpp - 日志系统实现

#include "logger.h"
#include <iostream>
#include <ctime>
#include <chrono>
#include <cstdio>
#include <cstring>

namespace mock {

const char* level_str(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default:              return "UNKNOWN";
    }
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger()
    : min_level_(LogLevel::INFO)
    , console_enabled_(true)
{
}

Logger::~Logger() {
    close();
}

void Logger::init(const std::string& log_file, LogLevel min_level, bool console_enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
    log_file_ = log_file;
    min_level_ = min_level;
    console_enabled_ = console_enabled;
    if (!log_file_.empty()) {
        file_.open(log_file_.c_str(), std::ios::out | std::ios::trunc);
    }
}

void Logger::set_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_level_ = level;
}

void Logger::log(LogLevel level, const char* file, int line, const std::string& msg) {
    // 低于最低级别直接丢弃
    if (static_cast<int>(level) < static_cast<int>(min_level_)) {
        return;
    }

    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch()).count() % 1000;
    std::tm tmv;
    ::localtime_r(&tt, &tmv);

    char ts[32];
    std::snprintf(ts, sizeof(ts), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                  tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                  tmv.tm_hour, tmv.tm_min, tmv.tm_sec, static_cast<int>(ms));

    // 只保留文件名 basename
    std::string fname = file ? file : "";
    size_t pos = fname.find_last_of("/\\");
    if (pos != std::string::npos) {
        fname = fname.substr(pos + 1);
    }

    // 去掉消息尾部换行（宏里可能带 std::endl）
    std::string body = msg;
    while (!body.empty() && (body[body.size() - 1] == '\n' || body[body.size() - 1] == '\r')) {
        body.erase(body.size() - 1);
    }

    std::ostringstream oss;
    oss << "[" << ts << "] [" << level_str(level) << "] ["
        << fname << ":" << line << "] " << body;

    std::lock_guard<std::mutex> lock(mutex_);
    if (console_enabled_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << oss.str() << std::endl;
        } else {
            std::cout << oss.str() << std::endl;
        }
    }
    if (file_.is_open()) {
        file_ << oss.str() << std::endl;
        file_.flush();
    }
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

} // namespace mock