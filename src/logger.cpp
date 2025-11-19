#include "logger.h"
#include <stdarg.h>

LogLevel Logger::currentLevel = LogLevel::INFO;

void Logger::init(unsigned long baudRate) {
    Serial.begin(baudRate);
    delay(1000);
}

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
}

const char* Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::DEBUG: return "DEBUG";
        default:              return "?????";
    }
}

const char* Logger::getTimestamp() {
    static char buffer[16];
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu.%03lu",
             hours % 24, minutes % 60, seconds % 60, ms % 1000);
    return buffer;
}

void Logger::log(LogLevel level, const char* msg) {
    if (level <= currentLevel) {
        Serial.print("[");
        Serial.print(getTimestamp());
        Serial.print("] ");
        Serial.print(levelToString(level));
        Serial.print(": ");
        Serial.println(msg);
    }
}

void Logger::logf(LogLevel level, const char* format, va_list args) {
    if (level <= currentLevel) {
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        log(level, buffer);
    }
}

// ERROR level
void Logger::error(const char* msg) {
    log(LogLevel::ERROR, msg);
}

void Logger::error(const String& msg) {
    log(LogLevel::ERROR, msg.c_str());
}

void Logger::errorf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logf(LogLevel::ERROR, format, args);
    va_end(args);
}

// WARN level
void Logger::warn(const char* msg) {
    log(LogLevel::WARN, msg);
}

void Logger::warn(const String& msg) {
    log(LogLevel::WARN, msg.c_str());
}

void Logger::warnf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logf(LogLevel::WARN, format, args);
    va_end(args);
}

// INFO level
void Logger::info(const char* msg) {
    log(LogLevel::INFO, msg);
}

void Logger::info(const String& msg) {
    log(LogLevel::INFO, msg.c_str());
}

void Logger::infof(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logf(LogLevel::INFO, format, args);
    va_end(args);
}

// DEBUG level
void Logger::debug(const char* msg) {
    log(LogLevel::DEBUG, msg);
}

void Logger::debug(const String& msg) {
    log(LogLevel::DEBUG, msg.c_str());
}

void Logger::debugf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logf(LogLevel::DEBUG, format, args);
    va_end(args);
}
