#include "../include/logging.h"
#include <time.h>
#include <string.h>
#include <errno.h>

// Global logging configuration
logging_config_t logging_config = {
    .level = LOG_LEVEL_INFO,
    .use_syslog = false,
    .use_file = false,
    .log_file = "",
    .log_fp = NULL,
    .debug_enabled = false
};

// Initialize logging system
int logging_init(int level, bool use_syslog, const char *log_file)
{
    logging_config.level = level;
    logging_config.use_syslog = use_syslog;
    logging_config.debug_enabled = (level >= LOG_LEVEL_DEBUG);
    
    if (log_file && strlen(log_file) > 0) {
        strncpy(logging_config.log_file, log_file, sizeof(logging_config.log_file) - 1);
        logging_config.log_file[sizeof(logging_config.log_file) - 1] = '\0';
        logging_config.use_file = true;
        
        // Open log file
        logging_config.log_fp = fopen(log_file, "a");
        if (!logging_config.log_fp) {
            fprintf(stderr, "Failed to open log file %s: %s\n", log_file, strerror(errno));
            return -1;
        }
    }
    
    if (use_syslog) {
        openlog("goodbyedpi", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    }
    
    log_info("Logging system initialized: level=%d, syslog=%s, file=%s", 
             level, use_syslog ? "yes" : "no", 
             log_file ? log_file : "none");
    
    return 0;
}

// Cleanup logging system
void logging_cleanup(void)
{
    if (logging_config.use_file && logging_config.log_fp) {
        fclose(logging_config.log_fp);
        logging_config.log_fp = NULL;
    }
    
    if (logging_config.use_syslog) {
        closelog();
    }
}

// Set log level
void logging_set_level(int level)
{
    logging_config.level = level;
    logging_config.debug_enabled = (level >= LOG_LEVEL_DEBUG);
}

// Set debug mode
void logging_set_debug(bool enable)
{
    logging_config.debug_enabled = enable;
}

// Core logging function
void log_message(int priority, const char *format, ...)
{
    va_list args;
    char timestamp[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    // Check if we should log this message
    if (priority > logging_config.level) {
        return;
    }
    
    // Format timestamp
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    va_start(args, format);
    
    if (logging_config.use_syslog) {
        // Log to syslog
        vsyslog(priority, format, args);
    } else {
        // Log to stderr or file
        FILE *fp = logging_config.use_file ? logging_config.log_fp : stderr;
        
        fprintf(fp, "[%s] ", timestamp);
        vfprintf(fp, format, args);
        fprintf(fp, "\n");
        fflush(fp);
    }
    
    va_end(args);
}

// Additional convenience functions
void log_error_func(const char *format, ...)
{
    va_list args;
    char buffer[1024];
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log_message(LOG_LEVEL_ERR, "ERROR: %s", buffer);
}

void log_info_func(const char *format, ...)
{
    va_list args;
    char buffer[1024];
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log_message(LOG_LEVEL_INFO, "INFO: %s", buffer);
}

void log_debug_func(const char *format, ...)
{
    if (!logging_config.debug_enabled) {
        return;
    }
    
    va_list args;
    char buffer[1024];
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    log_message(LOG_LEVEL_DEBUG, "DEBUG: %s", buffer);
}

// Performance monitoring
void log_packet_stats(uint64_t packets_processed, uint64_t packets_modified, 
                     uint64_t bytes_processed)
{
    log_info("STATS: processed=%lu, modified=%lu, bytes=%lu", 
             (unsigned long)packets_processed, 
             (unsigned long)packets_modified, 
             (unsigned long)bytes_processed);
}