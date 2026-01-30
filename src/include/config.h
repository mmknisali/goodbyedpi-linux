#ifndef CONFIG_H
#define CONFIG_H

#include "goodbyedpi.h"

// Default configuration values
#define DEFAULT_HTTP_FRAGMENT_SIZE      2
#define DEFAULT_HTTPS_FRAGMENT_SIZE     2
#define DEFAULT_DNS_PORT               53
#define DEFAULT_DNS_SERVER_V4           "1.1.1.1"    // Cloudflare DNS (better for Turkey)
#define DEFAULT_DNS_SERVER_TURKEY       "208.67.222.222"  // Turkey local DNS (when available)
#define DEFAULT_DNS_SERVER_V6           "2606:4700:4700::4700"  // Cloudflare IPv6
#define DEFAULT_PID_FILE                "/run/goodbyedpi.pid"
#define DEFAULT_LOG_FILE                "/var/log/goodbyedpi.log"
#define DEFAULT_CONFIG_FILE             "/etc/goodbyedpi/goodbyedpi.conf"
#define DEFAULT_BLACKLIST_FILE          "/etc/goodbyedpi/blacklist.txt"
#define DEFAULT_TURKEY_BLACKLIST_FILE   "/etc/goodbyedpi/blacklist-turkey.txt"  // Turkey-specific blocks
#define DEFAULT_MAX_PAYLOAD_SIZE        1200
#define TURKEY_MAX_FRAGMENT_SIZE        800    // Smaller for Turkish DPI
#define TURKEY_HTTP_FRAGMENT_SIZE       400    // More aggressive fragmentation
#define TURKEY_HTTPS_FRAGMENT_SIZE      200    // Smaller HTTPS fragments
#define MAX_HOSTNAME_LEN               256

// Configuration parsing
typedef struct {
    char key[64];
    char value[256];
} config_line_t;

// Configuration loading and saving
int config_init(goodbyedpi_config_t *cfg);
int config_load_file(const char *filename, goodbyedpi_config_t *cfg);
int config_save_file(const char *filename, const goodbyedpi_config_t *cfg);
int config_parse_line(const char *line, config_line_t *config_line);
int config_set_value(goodbyedpi_config_t *cfg, const char *key, const char *value);
int config_get_value(const goodbyedpi_config_t *cfg, const char *key, char *value, size_t value_len);

// Configuration validation
int config_validate(const goodbyedpi_config_t *cfg);
int config_validate_fragment_size(unsigned int size);
int config_validate_port(int port);
int config_validate_ip_address(const char *ip, bool is_ipv6);
int config_validate_file_path(const char *path, bool must_exist);

// Command line parsing
int config_parse_command_line(int argc, char *argv[], goodbyedpi_config_t *cfg);
void print_version(void);

// Legacy modes support
int config_apply_legacy_mode(int mode, goodbyedpi_config_t *cfg);

// Environment variables
int config_load_environment(goodbyedpi_config_t *cfg);

#endif // CONFIG_H