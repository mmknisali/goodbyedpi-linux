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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
    }
}

// Install signal handlers
int install_signal_handlers(void)
{
    struct sigaction sa;
    
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        log_error("Failed to install SIGINT handler");
        return -1;
    }
    
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        log_error("Failed to install SIGTERM handler");
        return -1;
    }
    
    return 0;
}

// Daemonize
int daemonize(void)
{
    pid_t pid = fork();
    
    if (pid < 0) {
        log_error("Failed to fork daemon");
        return -1;
    }
    
    if (pid > 0) {
        // Parent process exits
        exit(EXIT_SUCCESS);
    }
    
    // Child process continues
    if (setsid() < 0) {
        log_error("Failed to create new session");
        return -1;
    }
    
    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Redirect to /dev/null
    open("/dev/null", O_RDWR);
    (void)dup(0);
    (void)dup(0);
    
    return 0;
}

// Create PID file
int create_pid_file(const char *pidfile)
{
    FILE *fp = fopen(pidfile, "w");
    if (!fp) {
        log_error("Failed to create PID file %s: %s", pidfile, strerror(errno));
        return -1;
    }
    
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
    
    log_info("Created PID file: %s", pidfile);
    return 0;
}

// Remove PID file
int remove_pid_file(const char *pidfile)
{
    if (unlink(pidfile) == 0) {
        log_info("Removed PID file: %s", pidfile);
        return 0;
    }
    
    log_error("Failed to remove PID file %s: %s", pidfile, strerror(errno));
    return -1;
}

// Get running state
bool is_running(void)
{
    return running;
}

// Stop running
void stop_running(void)
{
    running = 0;
}