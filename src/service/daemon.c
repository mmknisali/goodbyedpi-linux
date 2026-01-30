#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

// Global flag for graceful shutdown
static volatile int running = 1;

// Signal handler for graceful shutdown
void signal_handler(int sig)
{
    switch (sig) {
        case SIGINT:
            log_info("Received SIGINT, shutting down gracefully");
            running = 0;
            break;
        case SIGTERM:
            log_info("Received SIGTERM, shutting down gracefully");
            running = 0;
            break;
        case SIGHUP:
            log_info("Received SIGHUP, reloading configuration");
            // TODO: Reload configuration
            break;
        case SIGUSR1:
            log_info("Received SIGUSR1, toggling debug mode");
            logging_set_debug(!logging_config.debug_enabled);
            break;
        default:
            log_info("Received signal %d", sig);
            break;
    }
}

// Install signal handlers
int install_signal_handlers(void)
{
    struct sigaction sa;
    
    // Setup signal handler structure
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // Restart interrupted system calls
    
    // Install signal handlers
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        log_error("Failed to install SIGINT handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        log_error("Failed to install SIGTERM handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        log_error("Failed to install SIGHUP handler: %s", strerror(errno));
        return -1;
    }
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        log_error("Failed to install SIGUSR1 handler: %s", strerror(errno));
        return -1;
    }
    
    // Ignore SIGPIPE (can happen when writing to closed sockets)
    signal(SIGPIPE, SIG_IGN);
    
    log_info("Signal handlers installed");
    return 0;
}

// Daemonize the process
int daemonize(void)
{
    pid_t pid;
    int fd;
    
    // First fork
    pid = fork();
    if (pid < 0) {
        log_error("First fork failed: %s", strerror(errno));
        return -1;
    }
    
    if (pid > 0) {
        // Parent process exits
        exit(EXIT_SUCCESS);
    }
    
    // Create new session
    if (setsid() < 0) {
        log_error("Failed to create new session: %s", strerror(errno));
        return -1;
    }
    
    // Second fork
    pid = fork();
    if (pid < 0) {
        log_error("Second fork failed: %s", strerror(errno));
        return -1;
    }
    
    if (pid > 0) {
        // Second parent exits
        exit(EXIT_SUCCESS);
    }
    
    // Change working directory to root
    if (chdir("/") < 0) {
        log_error("Failed to change directory to /: %s", strerror(errno));
        return -1;
    }
    
    // Set file mode creation mask
    umask(0);
    
    // Close all file descriptors
    for (fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
        close(fd);
    }
    
    // Reopen standard file descriptors
    fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
    
    log_info("Process daemonized successfully");
    return 0;
}

// Create PID file
int create_pid_file(const char *pidfile)
{
    FILE *fp;
    pid_t pid;
    
    if (!pidfile) {
        return -1;
    }
    
    // Check if PID file already exists
    fp = fopen(pidfile, "r");
    if (fp) {
        int existing_pid;
        if (fscanf(fp, "%d", &existing_pid) == 1) {
            // Check if process is still running
            if (kill(existing_pid, 0) == 0) {
                log_error("GoodbyeDPI is already running with PID %d", existing_pid);
                fclose(fp);
                return -1;
            }
        }
        fclose(fp);
    }
    
    // Create new PID file
    fp = fopen(pidfile, "w");
    if (!fp) {
        log_error("Cannot create PID file %s: %s", pidfile, strerror(errno));
        return -1;
    }
    
    pid = getpid();
    fprintf(fp, "%d\n", pid);
    fclose(fp);
    
    log_info("Created PID file %s with PID %d", pidfile, pid);
    return 0;
}

// Remove PID file
int remove_pid_file(const char *pidfile)
{
    if (!pidfile) {
        return -1;
    }
    
    if (unlink(pidfile) == 0) {
        log_info("Removed PID file %s", pidfile);
        return 0;
    } else {
        log_error("Failed to remove PID file %s: %s", pidfile, strerror(errno));
        return -1;
    }
}



// Get running state
int is_running(void)
{
    return running;
}

// Set running state to stop
void stop_running(void)
{
    running = 0;
}

// Forward declarations for compatibility
void signal_handler(int sig);
int install_signal_handlers(void);