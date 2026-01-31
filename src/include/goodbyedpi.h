#ifndef GOODBYEDPI_H
#define GOODBYEDPI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

// Constants
#define GOODBYEDPI_VERSION "v0.2.3rc3-linux"
#define MAX_PACKET_SIZE 65536
#define MAX_HOSTNAME_LEN 256
#define MAX_FILTER_LEN 2048
#define HOST_MAXLEN 253

// Error codes
typedef enum {
    ERROR_SUCCESS = 0,
    ERROR_DEFAULT = 1,
    ERROR_PORT_BOUNDS,
    ERROR_DNS_V4_ADDR,
    ERROR_DNS_V6_ADDR,
    ERROR_DNS_V4_PORT,
    ERROR_DNS_V6_PORT,
    ERROR_BLACKLIST_LOAD,
    ERROR_AUTOTTL,
    ERROR_ATOUSI,
    ERROR_AUTOB,
    ERROR_NETFILTER_INIT,
    ERROR_CONFIG_PARSE,
    ERROR_PERMISSION_DENIED
} error_code_t;

// Packet types
typedef enum {
    PACKET_UNKNOWN,
    PACKET_IPV4_TCP,
    PACKET_IPV4_TCP_DATA,
    PACKET_IPV4_UDP_DATA,
    PACKET_IPV6_TCP,
    PACKET_IPV6_TCP_DATA,
    PACKET_IPV6_UDP_DATA
} packet_type_t;

// Direction
typedef enum {
    DIRECTION_UNKNOWN,
    DIRECTION_OUTBOUND,
    DIRECTION_INBOUND
} packet_direction_t;

// Cross-platform packet structure
typedef struct {
    bool is_ipv6;
    bool is_outbound;
    packet_direction_t direction;
    packet_type_t type;
    uint8_t ttl;
    uint32_t src_ip[4];  // IPv4: first element only
    uint32_t dst_ip[4];  // IPv4: first element only
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t *payload;
    size_t payload_len;
    uint8_t *headers;
    size_t headers_len;
    uint32_t nfqueue_id;  // Netfilter queue specific
    void *raw_packet;    // Raw packet data for reinjection
    size_t raw_packet_len;
} packet_t;

// Connection tracking structures
typedef struct {
    bool valid;
    uint32_t src_addr[4];
    uint32_t dst_addr[4];
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t ttl;
    uint8_t protocol;
    time_t last_seen;
} conntrack_info_t;

typedef struct {
    bool valid;
    uint32_t ttl;
    uint8_t distance;
    time_t last_seen;
} tcp_conntrack_info_t;

// DNS tracking
typedef struct {
    bool valid;
    uint32_t client_ip[4];
    uint32_t dns_server_ip[4];
    uint16_t client_port;
    uint16_t dns_server_port;
    time_t timestamp;
} dns_conntrack_info_t;

// Evasion techniques configuration
typedef struct {
    // Fragmentation settings
    unsigned int http_fragment_size;
    unsigned int https_fragment_size;
    bool native_fragmentation;
    bool reverse_fragmentation;
    bool fragment_http_persistent;
    bool fragment_http_persistent_nowait;
    
    // Evasion settings
    bool host_mixedcase;
    bool additional_space;
    bool host_removespace;
    bool wrong_checksum;
    bool wrong_sequence;
    bool auto_ttl;
    bool fake_packet;
    
    // TTL settings
    uint8_t ttl_of_fake_packet;
    uint8_t ttl_min_nhops;
    uint8_t auto_ttl_1;
    uint8_t auto_ttl_2;
    uint8_t auto_ttl_max;
    
    // Header manipulation
    bool host_uppercase;
    bool host_lowercase_mixed;
    
    // Blacklist/whitelist
    bool enable_blacklist;
    bool enable_whitelist;
    char blacklist_file[256];
    char whitelist_file[256];
    bool allow_no_sni;
    bool fragment_by_sni;
    
    // DNS settings
    bool dns_redirect_ipv4;
    bool dns_redirect_ipv6;
    char dns_server_v4[INET6_ADDRSTRLEN];
    char dns_server_v6[INET6_ADDRSTRLEN];
    uint16_t dns_port_v4;
    uint16_t dns_port_v6;
    
    // General settings
    bool daemon_mode;
    bool systemd_mode;
    bool debug_mode;
    bool verbose_mode;
    bool block_quic;
    unsigned int max_payload_size;
    char pid_file[256];
    char log_file[256];
    
    // Network interface
    char interface[32];
    unsigned int additional_ports[16];
    size_t additional_ports_count;
    uint16_t ip_ids[32];
    uint16_t nfqueue_num;
    size_t ip_ids_count;
} goodbyedpi_config_t;

// Function declarations
extern goodbyedpi_config_t config;

// Core functions
int goodbyedpi_init(int argc, char *argv[]);
int goodbyedpi_run(void);
void goodbyedpi_cleanup(void);
void signal_handler(int sig);

// Packet processing
int packet_process(packet_t *packet);
int packet_parse(const uint8_t *data, size_t len, packet_t *packet);
int packet_reinject(const packet_t *packet, const uint8_t *modified_data, size_t modified_len);

// Evasion functions
int evasion_fragment_packet(packet_t *packet, unsigned int fragment_size);
int evasion_modify_headers(packet_t *packet);
int evasion_inject_fake_packet(const packet_t *packet);
int evasion_extract_sni(const uint8_t *tls_data, size_t tls_len, char *hostname, size_t hostname_len);

// Connection tracking
int conntrack_add(const packet_t *packet);
int conntrack_lookup(const packet_t *packet, conntrack_info_t *info);
int conntrack_cleanup(void);
int ttl_track_update(const packet_t *packet, uint8_t ttl);
uint8_t ttl_get_auto_ttl(uint8_t connection_ttl, uint8_t ttl_1, uint8_t ttl_2, uint8_t ttl_min, uint8_t ttl_max);

// DNS functions
int dns_redirect_init(void);
int dns_redirect_process_packet(packet_t *packet);
int dns_flush_cache(void);

// Utility functions
uint16_t atousi(const char *str, const char *msg);
uint8_t atoub(const char *str, const char *msg);
void mix_case(char *str, size_t len);
int extract_sni(const char *pktdata, unsigned int pktlen, char **hostnameaddr, unsigned int *hostnamelen);
int find_header_and_get_info(const char *pktdata, unsigned int pktlen,
                           const char *hdrname, char **hdrnameaddr,
                           char **hdrvalueaddr, unsigned int *hdrvaluelen);

// Logging functions
void log_message(int priority, const char *format, ...);
void log_error(const char *format, ...);
void log_info(const char *format, ...);
void log_debug(const char *format, ...);

// Service functions
int daemonize(void);
int create_pid_file(const char *pidfile);
int remove_pid_file(const char *pidfile);
int setup_privileges(void);

// Network utilities
int setup_netfilter_queue(void);
int setup_raw_socket(void);
int netlink_receive_packet(uint8_t *buffer, size_t buffer_len, uint32_t *packet_id);
int netlink_send_verdict(uint32_t packet_id, int verdict, const uint8_t *data, size_t data_len);

// Configuration
int config_load_defaults(void);
int config_parse_file(const char *filename);
int config_parse_args(int argc, char *argv[]);
void config_print(void);

// Missing function declarations
// From string_utils.c
void string_to_upper(char *str);
void mix_case(char *str, size_t len);
void safe_string_copy(char *dest, const char *src, size_t dest_size);

// From net_utils.c  
int parse_ipv4_address(const char *ip_str, uint32_t *ip_addr);

// From raw_socket.c
int send_raw_packet(const uint8_t *packet_data, size_t packet_len, bool is_ipv6);

// From header_mangle.c
int modify_http_headers(packet_t *packet);
int modify_tcp_headers(packet_t *packet);

// From sni_extractor.c
int parse_sni_extension(const uint8_t *ext_data, size_t ext_len, char *hostname, size_t hostname_len);

// From packet parsing
bool packet_is_tcp(const packet_t *packet);

// From sni_extractor.c
int parse_sni_extension(const uint8_t *ext_data, size_t ext_len, char *hostname, size_t hostname_len);

// From turkey_specific.c
int turkey_header_obfuscation(packet_t *packet);
int turkey_tls_obfuscation(packet_t *packet);
int turkey_enhanced_fragmentation(packet_t *packet);
bool turkey_is_blocked_service(const packet_t *packet);
int turkey_ttl_manipulation(packet_t *packet);
int turkey_generate_fake_service_traffic(const packet_t *packet);
int turkey_apply_all_evasion(packet_t *packet);
bool turkey_should_apply_techniques(const packet_t *packet);
unsigned int turkey_get_optimal_fragment_size(const packet_t *packet);
char *extract_sni_from_packet(const packet_t *packet);

// Additional Linux-specific includes
#include <libgen.h>

/* Turkey-specific evasion constants */
#define TURKEY_HTTP_FRAGMENT_SIZE 2
#define TURKEY_HTTPS_FRAGMENT_SIZE 2
#define TURKEY_MAX_FRAGMENT_SIZE 5
#define DEFAULT_HTTP_FRAGMENT_SIZE 40
#define DEFAULT_HTTPS_FRAGMENT_SIZE 40

#endif // GOODBYEDPI_H
