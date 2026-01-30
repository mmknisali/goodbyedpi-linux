#define _GNU_SOURCE
#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <sys/resource.h>

// Original privilege information
static struct {
    uid_t uid;
    gid_t gid;
    bool privilege_dropped;
} original_privs = {0};

// Save original privileges
int setup_privileges(void)
{
    original_privs.uid = getuid();
    original_privs.gid = getgid();
    original_privs.privilege_dropped = false;
    
    log_info("Current privileges: uid=%d, gid=%d", original_privs.uid, original_privs.gid);
    log_info("Effective privileges: euid=%d, egid=%d", geteuid(), getegid());
    
    return 0;
}

// Check if running as root
bool is_running_as_root(void)
{
    return geteuid() == 0;
}

// Drop privileges to unprivileged user
int drop_privileges(const char *username)
{
    if (original_privs.privilege_dropped) {
        log_warning("Privileges already dropped");
        return 0;
    }
    
    if (!is_running_as_root()) {
        log_info("Not running as root, no privileges to drop");
        return 0;
    }
    
    struct passwd *pw = NULL;
    
    if (username && strlen(username) > 0) {
        pw = getpwnam(username);
        if (!pw) {
            log_error("User '%s' not found: %s", username, strerror(errno));
            return -1;
        }
    } else {
        // Try 'nobody' user
        pw = getpwnam("nobody");
        if (!pw) {
            log_warning("User 'nobody' not found, keeping current privileges");
            return 0;
        }
    }
    
    // Drop supplementary groups first
    if (setgroups(0, NULL) != 0) {
        log_error("Failed to drop supplementary groups: %s", strerror(errno));
        return -1;
    }
    
    // Set GID
    if (setgid(pw->pw_gid) != 0) {
        log_error("Failed to set GID to %d: %s", pw->pw_gid, strerror(errno));
        return -1;
    }
    
    // Set UID
    if (setuid(pw->pw_uid) != 0) {
        log_error("Failed to set UID to %d: %s", pw->pw_uid, strerror(errno));
        return -1;
    }
    
    original_privs.privilege_dropped = true;
    
    log_info("Successfully dropped privileges to user '%s' (uid=%d, gid=%d)", 
             username ? username : "nobody", pw->pw_uid, pw->pw_gid);
    
    return 0;
}

// Restore original privileges (requires root privileges again)
int restore_privileges(void)
{
    if (!original_privs.privilege_dropped) {
        log_info("Privileges were not dropped, nothing to restore");
        return 0;
    }
    
    // This would typically require SUID root or similar mechanism
    // For now, just log the attempt
    log_warning("Privilege restoration not implemented - requires external mechanism");
    
    return -1;
}

// Check if we have required capabilities for network operations
bool has_network_capabilities(void)
{
    // Check if we can create raw sockets (requires root or CAP_NET_RAW)
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        log_debug("Cannot create IPv4 raw socket: %s", strerror(errno));
        return false;
    }
    close(sock);
    
    sock = socket(AF_INET6, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        log_debug("Cannot create IPv6 raw socket: %s", strerror(errno));
        return false;
    }
    close(sock);
    
    return true;
}

// Check if we can modify firewall rules
bool has_firewall_capabilities(void)
{
    // This would check for iptables/nftables access
    // For now, just check if we're root
    return is_running_as_root();
}

// Get privilege information
void get_privilege_info(uid_t *uid, gid_t *gid, bool *is_privileged)
{
    if (uid) *uid = getuid();
    if (gid) *gid = getgid();
    if (is_privileged) *is_privileged = is_running_as_root();
}

// Log current privilege status
void log_privilege_status(void)
{
    uid_t ruid, euid, suid;
    gid_t rgid, egid, sgid;
    
    getresuid(&ruid, &euid, &suid);
    getresgid(&rgid, &egid, &sgid);
    
    log_info("Privilege status:");
    log_info("  Real UID: %d, Effective UID: %d, Saved UID: %d", ruid, euid, suid);
    log_info("  Real GID: %d, Effective GID: %d, Saved GID: %d", rgid, egid, sgid);
    log_info("  Running as root: %s", is_running_as_root() ? "Yes" : "No");
    log_info("  Has network capabilities: %s", 
             has_network_capabilities() ? "Yes" : "No");
    log_info("  Has firewall capabilities: %s", 
             has_firewall_capabilities() ? "Yes" : "No");
}

// Check if specific file is accessible
bool can_access_file(const char *filename, int mode)
{
    return access(filename, mode) == 0;
}

// Check if we can write to PID file directory
bool can_write_pid_file(const char *pid_file)
{
    if (!pid_file) return false;
    
    // Extract directory from PID file path
    char *dir_copy = strdup(pid_file);
    if (!dir_copy) return false;
    
    char *dir = dirname(dir_copy);
    bool accessible = can_access_file(dir, W_OK);
    
    free(dir_copy);
    return accessible;
}

// Check if we can write to log file
bool can_write_log_file(const char *log_file)
{
    if (!log_file) return false;
    
    // Check if we can write to the file or its directory
    if (can_access_file(log_file, W_OK)) {
        return true;
    }
    
    // Check if we can create the file in its directory
    char *dir_copy = strdup(log_file);
    if (!dir_copy) return false;
    
    char *dir = dirname(dir_copy);
    bool accessible = can_access_file(dir, W_OK);
    
    free(dir_copy);
    return accessible;
}

// Validate that we have all required privileges
int validate_privileges(void)
{
    int issues = 0;
    
    log_info("Validating required privileges...");
    
    // Check network capabilities
    if (!has_network_capabilities()) {
        log_error("Missing network capabilities (need root or CAP_NET_RAW)");
        issues++;
    }
    
    // Check if we can write required files
    if (config.pid_file[0] && !can_write_pid_file(config.pid_file)) {
        log_error("Cannot write PID file: %s", config.pid_file);
        issues++;
    }
    
    if (config.log_file[0] && !can_write_log_file(config.log_file)) {
        log_error("Cannot write log file: %s", config.log_file);
        issues++;
    }
    
    // Check firewall capabilities (optional, but warn if missing)
    if (!has_firewall_capabilities()) {
        log_warning("Missing firewall capabilities (firewall rules must be set up manually)");
    }
    
    if (issues > 0) {
        log_error("Privilege validation failed with %d issues", issues);
        return -1;
    }
    
    log_info("Privilege validation successful");
    return 0;
}