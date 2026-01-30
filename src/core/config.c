#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

// Global instance
goodbyedpi_config_t config;

// Initialize configuration with defaults
int config_init(goodbyedpi_config_t *cfg)
{
    if (!cfg) {
        return -1;
    }
    
    memset(cfg, 0, sizeof(goodbyedpi_config_t));
    
    // Fragmentation defaults
    cfg->http_fragment_size = DEFAULT_HTTP_FRAGMENT_SIZE;
    cfg->https_fragment_size = DEFAULT_HTTPS_FRAGMENT_SIZE;
    cfg->native_fragmentation = true;
    cfg->reverse_fragmentation = false;
    cfg->fragment_http_persistent = false;
    cfg->fragment_http_persistent_nowait = false;
    
    // Evasion defaults
    cfg->host_mixedcase = false;
    cfg->additional_space = false;
    cfg->host_removespace = false;
    cfg->wrong_checksum = false;
    cfg->wrong_sequence = false;
    cfg->auto_ttl = false;
    cfg->fake_packet = false;
    
    // TTL defaults
    cfg->ttl_of_fake_packet = 64;
    cfg->ttl_min_nhops = 3;
    cfg->auto_ttl_1 = 1;
    cfg->auto_ttl_2 = 4;
    cfg->auto_ttl_max = 10;
    
    // Header manipulation defaults
    cfg->host_uppercase = false;
    cfg->host_lowercase_mixed = false;
    
    // DNS defaults
    cfg->dns_redirect_ipv4 = false;
    cfg->dns_redirect_ipv6 = false;
    strncpy(cfg->dns_server_v4, DEFAULT_DNS_SERVER_V4, sizeof(cfg->dns_server_v4) - 1);
    strncpy(cfg->dns_server_v6, DEFAULT_DNS_SERVER_V6, sizeof(cfg->dns_server_v6) - 1);
    cfg->dns_port_v4 = DEFAULT_DNS_PORT;
    cfg->dns_port_v6 = DEFAULT_DNS_PORT;
    
    // General defaults
    cfg->daemon_mode = false;
    cfg->systemd_mode = false;
    cfg->debug_mode = false;
    cfg->verbose_mode = false;
    cfg->block_quic = false;
    cfg->max_payload_size = DEFAULT_MAX_PAYLOAD_SIZE;
    
    // File paths
    strncpy(cfg->pid_file, DEFAULT_PID_FILE, sizeof(cfg->pid_file) - 1);
    strncpy(cfg->log_file, DEFAULT_LOG_FILE, sizeof(cfg->log_file) - 1);
    
    // Network interface (empty = all interfaces)
    cfg->interface[0] = '\0';
    
    return 0;
}

// Parse configuration file line
int config_parse_line(const char *line, config_line_t *config_line)
{
    const char *equals_pos;
    size_t key_len, value_len;
    
    if (!line || !config_line) {
        return -1;
    }
    
    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == '#' || line[0] == '\n') {
        return 1;  // Skip line
    }
    
    // Find '=' separator
    equals_pos = strchr(line, '=');
    if (!equals_pos) {
        return -1;
    }
    
    key_len = equals_pos - line;
    value_len = strlen(equals_pos + 1);
    
    // Copy key
    if (key_len >= sizeof(config_line->key)) {
        return -1;
    }
    strncpy(config_line->key, line, key_len);
    config_line->key[key_len] = '\0';
    
    // Copy value (strip whitespace)
    const char *value_start = equals_pos + 1;
    while (*value_start == ' ' || *value_start == '\t') {
        value_start++;
    }
    
    if (strlen(value_start) >= sizeof(config_line->value)) {
        return -1;
    }
    strncpy(config_line->value, value_start, sizeof(config_line->value) - 1);
    config_line->value[sizeof(config_line->value) - 1] = '\0';
    
    // Remove trailing whitespace
    char *end = config_line->value + strlen(config_line->value) - 1;
    while (end > config_line->value && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    
    return 0;
}

// Set configuration value
int config_set_value(goodbyedpi_config_t *cfg, const char *key, const char *value)
{
    if (!cfg || !key || !value) {
        return -1;
    }
    
    if (strcmp(key, "http_fragment_size") == 0) {
        cfg->http_fragment_size = (unsigned int)atoi(value);
    } else if (strcmp(key, "https_fragment_size") == 0) {
        cfg->https_fragment_size = (unsigned int)atoi(value);
    } else if (strcmp(key, "native_fragmentation") == 0) {
        cfg->native_fragmentation = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "reverse_fragmentation") == 0) {
        cfg->reverse_fragmentation = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "host_mixedcase") == 0) {
        cfg->host_mixedcase = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "additional_space") == 0) {
        cfg->additional_space = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "host_removespace") == 0) {
        cfg->host_removespace = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "wrong_checksum") == 0) {
        cfg->wrong_checksum = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "wrong_sequence") == 0) {
        cfg->wrong_sequence = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "auto_ttl") == 0) {
        cfg->auto_ttl = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "fake_packet") == 0) {
        cfg->fake_packet = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "dns_server_v4") == 0) {
        strncpy(cfg->dns_server_v4, value, sizeof(cfg->dns_server_v4) - 1);
    } else if (strcmp(key, "dns_server_v6") == 0) {
        strncpy(cfg->dns_server_v6, value, sizeof(cfg->dns_server_v6) - 1);
    } else if (strcmp(key, "dns_redirect_ipv4") == 0) {
        cfg->dns_redirect_ipv4 = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "dns_redirect_ipv6") == 0) {
        cfg->dns_redirect_ipv6 = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "debug") == 0) {
        cfg->debug_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "verbose") == 0) {
        cfg->verbose_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "daemon") == 0) {
        cfg->daemon_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else {
        log_debug("Unknown configuration key: %s", key);
        return -1;
    }
    
    log_debug("Config set: %s = %s", key, value);
    return 0;
}

// Load configuration from file
int config_load_file(const char *filename, goodbyedpi_config_t *cfg)
{
    FILE *fp;
    char line[512];
    config_line_t config_line;
    int line_num = 0;
    
    if (!filename || !cfg) {
        return -1;
    }
    
    fp = fopen(filename, "r");
    if (!fp) {
        log_error("Cannot open config file %s: %s", filename, strerror(errno));
        return -1;
    }
    
    log_info("Loading configuration from %s", filename);
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        int result = config_parse_line(line, &config_line);
        if (result == 0) {
            config_set_value(cfg, config_line.key, config_line.value);
        } else if (result < 0) {
            log_error("Error parsing config file %s at line %d", filename, line_num);
            fclose(fp);
            return -1;
        }
    }
    
    fclose(fp);
    log_info("Configuration loaded successfully");
    
    return 0;
}

// Validate configuration
int config_validate(const goodbyedpi_config_t *cfg)
{
    if (!cfg) {
        return -1;
    }
    
    // Validate fragment sizes
    if (cfg->http_fragment_size > 1500) {
        log_error("HTTP fragment size too large: %u (max 1500)", cfg->http_fragment_size);
        return -1;
    }
    
    if (cfg->https_fragment_size > 1500) {
        log_error("HTTPS fragment size too large: %u (max 1500)", cfg->https_fragment_size);
        return -1;
    }
    
    // Validate TTL values
    if (cfg->ttl_of_fake_packet == 0 || cfg->ttl_of_fake_packet > 255) {
        log_error("Invalid TTL of fake packet: %u", cfg->ttl_of_fake_packet);
        return -1;
    }
    
    // Validate DNS servers
    if (cfg->dns_redirect_ipv4 && strlen(cfg->dns_server_v4) == 0) {
        log_error("DNS IPv4 redirection enabled but no server specified");
        return -1;
    }
    
    if (cfg->dns_redirect_ipv6 && strlen(cfg->dns_server_v6) == 0) {
        log_error("DNS IPv6 redirection enabled but no server specified");
        return -1;
    }
    
    // Validate port ranges
    if (cfg->dns_port_v4 == 0 || cfg->dns_port_v4 > 65535) {
        log_error("Invalid DNS IPv4 port: %u", cfg->dns_port_v4);
        return -1;
    }
    
    if (cfg->dns_port_v6 == 0 || cfg->dns_port_v6 > 65535) {
        log_error("Invalid DNS IPv6 port: %u", cfg->dns_port_v6);
        return -1;
    }
    
    log_info("Configuration validation passed");
    return 0;
}

// Print configuration
void config_print(void)
{
    log_info("=== GoodbyeDPI Configuration ===");
    log_info("Fragmentation: HTTP=%u, HTTPS=%u", config.http_fragment_size, config.https_fragment_size);
    log_info("Native fragmentation: %s", config.native_fragmentation ? "yes" : "no");
    log_info("Reverse fragmentation: %s", config.reverse_fragmentation ? "yes" : "no");
    log_info("Host mixed case: %s", config.host_mixedcase ? "yes" : "no");
    log_info("Additional space: %s", config.additional_space ? "yes" : "no");
    log_info("Host remove space: %s", config.host_removespace ? "yes" : "no");
    log_info("Wrong checksum: %s", config.wrong_checksum ? "yes" : "no");
    log_info("Wrong sequence: %s", config.wrong_sequence ? "yes" : "no");
    log_info("Auto TTL: %s", config.auto_ttl ? "yes" : "no");
    log_info("Fake packet: %s", config.fake_packet ? "yes" : "no");
    log_info("Block QUIC: %s", config.block_quic ? "yes" : "no");
    log_info("Debug mode: %s", config.debug_mode ? "yes" : "no");
    log_info("Daemon mode: %s", config.daemon_mode ? "yes" : "no");
    log_info("Max payload size: %u", config.max_payload_size);
    
    if (config.dns_redirect_ipv4) {
        log_info("DNS IPv4 redirection: %s:%u", config.dns_server_v4, config.dns_port_v4);
    }
    if (config.dns_redirect_ipv6) {
        log_info("DNS IPv6 redirection: %s:%u", config.dns_server_v6, config.dns_port_v6);
    }
    
    log_info("================================");
}

// Load defaults into global config
int config_load_defaults(void)
{
    return config_init(&config);
}