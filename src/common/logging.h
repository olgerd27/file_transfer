#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

// Logging levels
#define LOG_LEVEL_NONE   0
#define LOG_LEVEL_ERROR  1
#define LOG_LEVEL_WARN   2
#define LOG_LEVEL_INFO   3
#define LOG_LEVEL_DEBUG  4

// Define the log level globally (can be overridden via the Makefile)
// To disable logging, set LOG_LEVEL_NONE value for GLOBAL_LOG_LEVEL;
// to enable the most detailed logging, set LOG_LEVEL_DEBUG value for GLOBAL_LOG_LEVEL.
#ifndef GLOBAL_LOG_LEVEL
#define GLOBAL_LOG_LEVEL LOG_LEVEL_INFO
#endif

// Macros for enabling/disabling specific logging types
// Debug messages for server
#ifndef LOG_TYPE_SERV
#define LOG_TYPE_SERV 1
#endif

// Debug messages for client
#ifndef LOG_TYPE_CLNT
#define LOG_TYPE_CLNT 0
#endif

// Debug messages for interaction operations
#ifndef LOG_TYPE_INTR
#define LOG_TYPE_INTR 0
#endif

// Debug messages for file type info
#ifndef LOG_TYPE_FTINF
#define LOG_TYPE_FTINF 0
#endif

// Debug messages for file selection
#ifndef LOG_TYPE_SLCT
#define LOG_TYPE_SLCT 1
#endif

// Debug messages for memory manipulation
#ifndef LOG_TYPE_MEM
#define LOG_TYPE_MEM 0
#endif

// Debug messages for file operations
#ifndef LOG_TYPE_FLOP
#define LOG_TYPE_FLOP 1
#endif

// String representations for log levels
static const char* log_level_str(int level)
{
    switch (level) {
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_DEBUG: return "DEBUG";
        default: return "UNKWN";
    }
}

// Get the current timestamp
static const char * get_timestamp()
{
    // Static buffer reused across calls.
    // Doesn't work correctly in a multi-threaded environment
    static char timestamp[64];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // to get milliseconds
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec, ts.tv_nsec / 1000000);
    return timestamp;
}

// General logging macro that checks the log level and the logging type
#define LOG(type, level, fmt, ...) \
    do { \
        if (type && level <= GLOBAL_LOG_LEVEL) \
            fprintf(stderr, "%s | %-5s | %s:%d, %s | " fmt "\n", \
              get_timestamp(), log_level_str(level), __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    } while(0)

#endif