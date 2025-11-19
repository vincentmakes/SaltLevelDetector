#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

enum class LogLevel : uint8_t {
    ERROR = 0,
    WARN = 1,
    INFO = 2,
    DEBUG = 3
};

class Logger {
public:
    static void init(unsigned long baudRate = 115200);
    static void setLevel(LogLevel level);
    
    // Logging methods
    static void error(const char* msg);
    static void error(const String& msg);
    static void errorf(const char* format, ...);
    
    static void warn(const char* msg);
    static void warn(const String& msg);
    static void warnf(const char* format, ...);
    
    static void info(const char* msg);
    static void info(const String& msg);
    static void infof(const char* format, ...);
    
    static void debug(const char* msg);
    static void debug(const String& msg);
    static void debugf(const char* format, ...);
    
private:
    static LogLevel currentLevel;
    
    static void log(LogLevel level, const char* msg);
    static void logf(LogLevel level, const char* format, va_list args);
    static const char* levelToString(LogLevel level);
    static const char* getTimestamp();
};

#endif // LOGGER_H
