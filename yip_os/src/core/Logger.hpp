#pragma once

#include <string>
#include <cstdio>
#include <mutex>

namespace YipOS {

class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERR,       // Not "ERROR" — Windows headers #define ERROR 0
        CRITICAL
    };

    static void Init(const std::string& logDirPath);
    static void Shutdown();

    static void Log(Level level, const std::string& message);
    static void Debug(const std::string& message);
    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);
    static void Critical(const std::string& message);

    static bool IsInitialized() { return initialized_; }
    static const std::string& GetLogPath() { return logPath_; }
    static void SetLogLevel(Level level);
    static Level StringToLevel(const std::string& str);
    static std::string LevelToString(Level level);

    // Optional UI callback — receives formatted log lines (thread-safe)
    using UICallback = void(*)(const std::string& line);
    static void SetUICallback(UICallback cb) { ui_callback_ = cb; }

private:
    static int logFd_;           // Raw POSIX file descriptor
    static std::string logPath_;
    static bool initialized_;
    static Level minLevel_;
    static std::mutex mutex_;
    static UICallback ui_callback_;
    static std::string GetTimeString();
    static std::string GetLevelString(Level level);
    static void WriteRaw(const char* data, size_t len);
};

} // namespace YipOS
