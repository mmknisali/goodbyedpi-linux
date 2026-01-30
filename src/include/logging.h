#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <stdint.h>
#include <stdbool.h>

// Log levels (compatible with syslog)
#define LOG_LEVEL_EMERG   0   // System is unusable
#define LOG_LEVEL_ALERT   1   // Action must be taken immediately
#define LOG_LEVEL_CRIT    2   // Critical conditions
#define LOG_LEVEL_ERR     3   // Error conditions
#define LOG_LEVEL_WARNING 4   // Warning conditions
#define LOG_LEVEL_NOTICE  5   // Normal but significant condition
#define LOG_LEVEL_INFO    6   // Informational messages
#define LOG_LEVEL_DEBUG   7   // Debug-level messages

// Logging configuration
typedef struct {
    int level;                  // Current log level
    bool use_syslog;           // Use syslog instead of stderr
    bool use_file;             // Log to file
    char log_file[256];        // Log file path
    FILE *log_fp;             // File pointer for log file
    bool debug_enabled;         // Debug mode flag
} logging_config_t;

// Global logging configuration
extern logging_config_t logging_config;

// Logging functions
int logging_init(int level, bool use_syslog, const char *log_file);
void logging_cleanup(void);
void logging_set_level(int level);
void logging_set_debug(bool enable);

// Core logging function
void log_message(int priority, const char *format, ...);

// Convenience macros
#define log_emerg(fmt, ...)   log_message(LOG_LEVEL_EMERG,   "EMERG: " fmt, ##__VA_ARGS__)
#define log_alert(fmt, ...)   log_message(LOG_LEVEL_ALERT,   "ALERT: " fmt, ##__VA_ARGS__)
#define log_crit(fmt, ...)    log_message(LOG_LEVEL_CRIT,    "CRIT: "  fmt, ##__VA_ARGS__)
#define log_error(fmt, ...)   log_message(LOG_LEVEL_ERR,     "ERROR: " fmt, ##__VA_ARGS__)
#define log_warning(fmt, ...) log_message(LOG_LEVEL_WARNING, "WARN: "  fmt, ##__VA_ARGS__)
#define log_notice(fmt, ...)  log_message(LOG_LEVEL_NOTICE,  "NOTICE: " fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)    log_message(LOG_LEVEL_INFO,    "INFO: "  fmt, ##__VA_ARGS__)
#define log_debug(fmt, ...)   do { \
    if (logging_config.debug_enabled) \
        log_message(LOG_LEVEL_DEBUG, "DEBUG: " fmt, ##__VA_ARGS__); \
} while(0)

// Additional convenience functions
void log_error_func(const char *format, ...);
void log_info_func(const char *format, ...);
void log_debug_func(const char *format, ...);

// Performance monitoring
void log_packet_stats(uint64_t packets_processed, uint64_t packets_modified, 
                     uint64_t bytes_processed);

#endif // LOGGING_H