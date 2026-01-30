#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#ifdef ENABLE_SYSTEMD
#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>
#endif

// Initialize systemd integration
int systemd_init(void)
{
#ifdef ENABLE_SYSTEMD
    // Notify systemd that we're starting up
    sd_notify(0, "READY=1");
    log_info("Systemd integration initialized");
    return 0;
#else
    log_debug("Systemd support not compiled in");
    return -1;
#endif
}

// Cleanup systemd integration
int systemd_cleanup(void)
{
#ifdef ENABLE_SYSTEMD
    // Notify systemd that we're shutting down
    sd_notify(0, "STOPPING=1");
    log_info("Systemd integration cleaned up");
    return 0;
#else
    return -1;
#endif
}

// Notify systemd about status change
int systemd_notify_status(const char *status)
{
#ifdef ENABLE_SYSTEMD
    if (!status) return -1;
    
    char notify_msg[256];
    snprintf(notify_msg, sizeof(notify_msg), "STATUS=%s", status);
    sd_notify(0, notify_msg);
    
    log_debug("Systemd status notification: %s", status);
    return 0;
#else
    return -1;
#endif
}

// Notify systemd about readiness
int systemd_notify_ready(void)
{
#ifdef ENABLE_SYSTEMD
    sd_notify(0, "READY=1");
    log_info("Notified systemd of readiness");
    return 0;
#else
    return -1;
#endif
}

// Notify systemd about stopping
int systemd_notify_stopping(void)
{
#ifdef ENABLE_SYSTEMD
    sd_notify(0, "STOPPING=1");
    log_info("Notified systemd of stopping");
    return 0;
#else
    return -1;
#endif
}

// Log to systemd journal
int systemd_log_message(int priority, const char *message)
{
#ifdef ENABLE_SYSTEMD
    if (!message) return -1;
    
    // Map our log levels to systemd priorities
    int sd_priority = LOG_INFO;
    switch (priority) {
        case LOG_LEVEL_EMERG:   sd_priority = LOG_EMERG; break;
        case LOG_LEVEL_ALERT:   sd_priority = LOG_ALERT; break;
        case LOG_LEVEL_CRIT:    sd_priority = LOG_CRIT; break;
        case LOG_LEVEL_ERR:     sd_priority = LOG_ERR; break;
        case LOG_LEVEL_WARNING: sd_priority = LOG_WARNING; break;
        case LOG_LEVEL_NOTICE:  sd_priority = LOG_NOTICE; break;
        case LOG_LEVEL_INFO:    sd_priority = LOG_INFO; break;
        case LOG_LEVEL_DEBUG:   sd_priority = LOG_DEBUG; break;
    }
    
    sd_journal_print(sd_priority, "%s", message);
    return 0;
#else
    return -1;
#endif
}

// Check if running under systemd
bool systemd_is_active(void)
{
#ifdef ENABLE_SYSTEMD
    // Check if we're running under systemd
    const char *watchdog = getenv("WATCHDOG_USEC");
    return (watchdog != NULL);
#else
    return false;
#endif
}

// Get systemd watchdog timeout
int systemd_get_watchdog_timeout(void)
{
#ifdef ENABLE_SYSTEMD
    const char *watchdog = getenv("WATCHDOG_USEC");
    if (watchdog) {
        return atoi(watchdog);
    }
    return 0;
#else
    return 0;
#endif
}

// Send watchdog ping
int systemd_watchdog_ping(void)
{
#ifdef ENABLE_SYSTEMD
    sd_notify(0, "WATCHDOG=1");
    log_debug("Sent systemd watchdog ping");
    return 0;
#else
    return -1;
#endif
}

// Create systemd watchdog file descriptor
int systemd_setup_watchdog(void)
{
#ifdef ENABLE_SYSTEMD
    int timeout = systemd_get_watchdog_timeout();
    if (timeout > 0) {
        log_info("Systemd watchdog enabled with timeout: %d us", timeout);
        return 0;
    }
    log_debug("Systemd watchdog not enabled");
    return -1;
#else
    return -1;
#endif
}

// Check if systemd integration is available
bool systemd_is_available(void)
{
#ifdef ENABLE_SYSTEMD
    return true;
#else
    return false;
#endif
}

// Get systemd unit name
const char *systemd_get_unit_name(void)
{
#ifdef ENABLE_SYSTEMD
    return getenv("SYSTEMD_UNIT");
#else
    return NULL;
#endif
}

// Get systemd service name
const char *systemd_get_service_name(void)
{
#ifdef ENABLE_SYSTEMD
    return getenv("SYSTEMD_SERVICE");
#else
    return NULL;
#endif
}

// Enable systemd socket activation support
int systemd_setup_socket_activation(void)
{
#ifdef ENABLE_SYSTEMD
    // This would handle socket activation if implemented
    log_debug("Systemd socket activation not implemented");
    return -1;
#else
    return -1;
#endif
}