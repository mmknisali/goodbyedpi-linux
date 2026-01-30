#include "include/goodbyedpi.h"
#include "include/logging.h"
#include "include/config.h"
#include "include/netfilter_capture.h"

// Function declarations from daemon.c
int install_signal_handlers(void);
int daemonize(void);
int create_pid_file(const char *pidfile);
int remove_pid_file(const char *pidfile);
bool is_running(void);
void stop_running(void);
int setup_privileges(void);
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

// Global variables
netfilter_context_t nfq_ctx;
uint64_t packets_processed = 0;
uint64_t packets_modified = 0;
uint64_t bytes_processed = 0;

// Main packet processing callback
static int packet_process_callback(struct nfq_q_handle *qh, 
                                struct nfgenmsg *nfmsg,
                                struct nfq_data *nfa,
                                void *data)
{
    packet_t packet;
    uint8_t *packet_data;
    uint32_t packet_len;
    uint32_t packet_id;
    int verdict = NF_ACCEPT;
    
    // Get packet metadata
    if (netfilter_get_packet_metadata(nfa, &packet_id, NULL, NULL, NULL, NULL) < 0) {
        log_debug("Failed to get packet metadata");
        return NF_ACCEPT;
    }
    
    // Get packet data
    if (netfilter_get_packet_data(nfa, &packet_data, &packet_len) < 0) {
        log_debug("Failed to get packet data");
        return NF_ACCEPT;
    }
    
    packets_processed++;
    bytes_processed += packet_len;
    
    // Parse packet
    if (packet_parse(packet_data, packet_len, &packet) < 0) {
        log_debug("Failed to parse packet");
        return NF_ACCEPT;
    }
    
    packet.nfqueue_id = packet_id;
    
    log_debug("Processing packet: ID=%u, len=%u", packet_id, packet_len);
    
    // Apply packet processing logic
    if (packet_process(&packet) == 0) {
        // Packet was modified
        packets_modified++;
        
        if (packet.raw_packet && packet.raw_packet_len > 0) {
            // Send modified packet
            verdict = netfilter_send_verdict(&nfq_ctx, packet_id, NF_ACCEPT, 
                                          packet.raw_packet, packet.raw_packet_len);
        }
    }
    
    // Cleanup
    packet_free(&packet);
    
    return verdict;
}

// Initialize netfilter with iptables rules
static int setup_firewall_rules(void)
{
    // TODO: Set up iptables/nftables rules to redirect traffic to NFQUEUE
    log_info("Setting up firewall rules");
    
    // For now, user must set up rules manually:
    // iptables -I OUTPUT -p tcp --dport 80 -j NFQUEUE --queue-num 0
    // iptables -I OUTPUT -p tcp --dport 443 -j NFQUEUE --queue-num 0
    // iptables -I INPUT -p tcp --sport 80 -j NFQUEUE --queue-num 0
    // iptables -I INPUT -p tcp --sport 443 -j NFQUEUE --queue-num 0
    
    log_warning("Note: Manual firewall rules required. See documentation.");
    
    return 0;
}

// Cleanup firewall rules
static int cleanup_firewall_rules(void)
{
    // TODO: Clean up iptables/nftables rules
    log_info("Cleaning up firewall rules");
    
    // For now, user must clean up manually
    
    return 0;
}

// Print usage information
void print_usage(const char *program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("GoodbyeDPI Linux - DPI bypass and circumvention utility\n\n");
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -V, --version           Show version information\n");
    printf("  -d, --daemon            Run as daemon\n");
    printf("  -c, --config FILE       Load configuration from file\n");
    printf("  -p, --pidfile FILE      PID file path (default: %s)\n", DEFAULT_PID_FILE);
    printf("  -l, --logfile FILE      Log file path\n");
    printf("  -v, --verbose           Enable verbose output\n");
    printf("  --debug                 Enable debug output\n");
    printf("  --syslog                Use syslog for logging\n");
    printf("\nFragmentation options:\n");
    printf("  -f, --fragment-http SIZE    HTTP fragment size\n");
    printf("  -e, --fragment-https SIZE   HTTPS fragment size\n");
    printf("  --native-frag               Use native fragmentation\n");
    printf("  --reverse-frag              Use reverse fragmentation\n");
    printf("\nHeader manipulation:\n");
    printf("  --host-mixedcase          Mix case in Host header\n");
    printf("  --additional-space         Add additional space\n");
    printf("  --host-removespace        Remove space after Host:\n");
    printf("\nDNS options:\n");
    printf("  --dns-redirect-v4 ADDR    Redirect IPv4 DNS to ADDR\n");
    printf("  --dns-redirect-v6 ADDR    Redirect IPv6 DNS to ADDR\n");
    printf("  --dns-port PORT           DNS port (default: 53)\n");
    printf("\nLegacy modes:\n");
    printf("  -1                     Legacy mode 1 (compatible)\n");
    printf("  -2                     Legacy mode 2 (HTTPS optimization)\n");
    printf("  -5                     Modern mode 5 (auto-TTL)\n");
    printf("  -6                     Modern mode 6 (wrong-seq)\n");
    printf("  -7                     Modern mode 7 (wrong-chksum)\n");
    printf("  -9                     Modern mode 9 (full features)\n");
    printf("\nNote: This tool requires root privileges for packet capture.\n");
}

// Parse command line arguments
static int parse_arguments(int argc, char *argv[], goodbyedpi_config_t *cfg)
{
    static struct option long_options[] = {
        {"help",             no_argument,       0, 'h'},
        {"version",          no_argument,       0, 'V'},
        {"daemon",           no_argument,       0, 'd'},
        {"config",           required_argument, 0, 'c'},
        {"pidfile",          required_argument, 0, 'p'},
        {"logfile",          required_argument, 0, 'l'},
        {"verbose",          no_argument,       0, 'v'},
        {"debug",            no_argument,       0, 1000},
        {"syslog",           no_argument,       0, 1001},
        {"fragment-http",     required_argument, 0, 1002},
        {"fragment-https",    required_argument, 0, 1003},
        {"native-frag",      no_argument,       0, 1004},
        {"reverse-frag",     no_argument,       0, 1005},
        {"host-mixedcase",    no_argument,       0, 1006},
        {"additional-space",  no_argument,       0, 1007},
        {"host-removespace", no_argument,       0, 1008},
        {"dns-redirect-v4",  required_argument, 0, 1009},
        {"dns-redirect-v6",  required_argument, 0, 1010},
        {"dns-port",         required_argument, 0, 1011},
        {0, 0, 0, 0}
    };
    
    int c, option_index = 0;
    
    while ((c = getopt_long(argc, argv, "hVdc:p:l:v123456789", 
                          long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print_usage(argv[0]);
                return 1;
                
            case 'V':
                printf("GoodbyeDPI Linux %s\n", GOODBYEDPI_VERSION);
                return 1;
                
            case 'd':
                cfg->daemon_mode = true;
                break;
                
            case 'c':
                if (config_load_file(optarg, cfg) < 0) {
                    fprintf(stderr, "Failed to load config file: %s\n", optarg);
                    return -1;
                }
                break;
                
            case 'p':
                strncpy(cfg->pid_file, optarg, sizeof(cfg->pid_file) - 1);
                cfg->pid_file[sizeof(cfg->pid_file) - 1] = '\0';
                break;
                
            case 'l':
                strncpy(cfg->log_file, optarg, sizeof(cfg->log_file) - 1);
                cfg->log_file[sizeof(cfg->log_file) - 1] = '\0';
                break;
                
            case 'v':
                cfg->verbose_mode = true;
                break;
                
            case '1':
                config_apply_legacy_mode(1, cfg);
                break;
            case '2':
                config_apply_legacy_mode(2, cfg);
                break;
            case '5':
                config_apply_legacy_mode(5, cfg);
                break;
            case '6':
                config_apply_legacy_mode(6, cfg);
                break;
            case '7':
                config_apply_legacy_mode(7, cfg);
                break;
            case '9':
                config_apply_legacy_mode(9, cfg);
                break;
                
            case 1000:
                cfg->debug_mode = true;
                break;
                
            case 1001:
                cfg->systemd_mode = true;
                break;
                
            case 1002:
                cfg->http_fragment_size = (unsigned int)atoi(optarg);
                break;
                
            case 1003:
                cfg->https_fragment_size = (unsigned int)atoi(optarg);
                break;
                
            case 1004:
                cfg->native_fragmentation = true;
                break;
                
            case 1005:
                cfg->reverse_fragmentation = true;
                break;
                
            case 1006:
                cfg->host_mixedcase = true;
                break;
                
            case 1007:
                cfg->additional_space = true;
                break;
                
            case 1008:
                cfg->host_removespace = true;
                break;
                
            case 1009:
                cfg->dns_redirect_ipv4 = true;
                strncpy(cfg->dns_server_v4, optarg, sizeof(cfg->dns_server_v4) - 1);
                cfg->dns_server_v4[sizeof(cfg->dns_server_v4) - 1] = '\0';
                break;
                
            case 1010:
                cfg->dns_redirect_ipv6 = true;
                strncpy(cfg->dns_server_v6, optarg, sizeof(cfg->dns_server_v6) - 1);
                cfg->dns_server_v6[sizeof(cfg->dns_server_v6) - 1] = '\0';
                break;
                
            case 1011:
                cfg->dns_port_v4 = cfg->dns_port_v6 = (uint16_t)atoi(optarg);
                break;
                
            case '?':
                fprintf(stderr, "Use -h or --help for usage information.\n");
                return -1;
                
            default:
                break;
        }
    }
    
    return 0;
}

// Apply legacy mode settings
int config_apply_legacy_mode(int mode, goodbyedpi_config_t *cfg)
{
    switch (mode) {
        case 1: // Compatible mode
            cfg->host_mixedcase = true;
            cfg->host_removespace = true;
            cfg->http_fragment_size = 2;
            cfg->https_fragment_size = 2;
            cfg->native_fragmentation = true;
            cfg->reverse_fragmentation = false;
            cfg->fragment_http_persistent = true;
            cfg->fragment_http_persistent_nowait = true;
            break;
            
        case 2: // HTTPS optimization
            cfg->host_mixedcase = true;
            cfg->host_removespace = true;
            cfg->http_fragment_size = 2;
            cfg->https_fragment_size = 40;
            cfg->native_fragmentation = true;
            cfg->reverse_fragmentation = false;
            cfg->fragment_http_persistent = true;
            cfg->fragment_http_persistent_nowait = true;
            break;
            
        case 5: // Auto-TTL mode
            cfg->http_fragment_size = 2;
            cfg->https_fragment_size = 2;
            cfg->native_fragmentation = true;
            cfg->reverse_fragmentation = true;
            cfg->auto_ttl = true;
            cfg->fake_packet = true;
            cfg->max_payload_size = 1200;
            break;
            
        case 6: // Wrong sequence mode
            cfg->http_fragment_size = 2;
            cfg->https_fragment_size = 2;
            cfg->native_fragmentation = true;
            cfg->reverse_fragmentation = true;
            cfg->wrong_sequence = true;
            cfg->fake_packet = true;
            cfg->max_payload_size = 1200;
            break;
            
        case 7: // Wrong checksum mode
            cfg->http_fragment_size = 2;
            cfg->https_fragment_size = 2;
            cfg->native_fragmentation = true;
            cfg->reverse_fragmentation = true;
            cfg->wrong_checksum = true;
            cfg->fake_packet = true;
            cfg->max_payload_size = 1200;
            break;
            
        case 9: // Full features mode (default)
            cfg->http_fragment_size = 2;
            cfg->https_fragment_size = 2;
            cfg->native_fragmentation = true;
            cfg->reverse_fragmentation = true;
            cfg->wrong_sequence = true;
            cfg->wrong_checksum = true;
            cfg->fake_packet = true;
            cfg->block_quic = true;
            cfg->max_payload_size = 1200;
            break;
            
        default:
            log_error("Unknown legacy mode: %d", mode);
            return -1;
    }
    
    log_info("Applied legacy mode %d", mode);
    return 0;
}

// Main function
int main(int argc, char *argv[])
{
    int result;
    
    printf("GoodbyeDPI %s\n", GOODBYEDPI_VERSION);
    printf("Linux DPI bypass and circumvention utility\n");
    printf("https://github.com/goodbyedpi-linux\n\n");
    
    // Initialize configuration with defaults
    if (config_load_defaults() < 0) {
        fprintf(stderr, "Failed to initialize configuration\n");
        return EXIT_FAILURE;
    }
    
    // Parse command line arguments
    result = parse_arguments(argc, argv, &config);
    if (result > 0) {
        return EXIT_SUCCESS;  // Help or version was shown
    } else if (result < 0) {
        fprintf(stderr, "Failed to parse command line arguments\n");
        return EXIT_FAILURE;
    }
    
    // Validate configuration
    if (config_validate(&config) < 0) {
        fprintf(stderr, "Configuration validation failed\n");
        return EXIT_FAILURE;
    }
    
    // Print configuration if verbose
    if (config.verbose_mode) {
        config_print();
    }
    
    // Initialize logging
    int log_level = config.debug_mode ? LOG_LEVEL_DEBUG : 
                   config.verbose_mode ? LOG_LEVEL_INFO : LOG_LEVEL_NOTICE;
    
    if (logging_init(log_level, config.systemd_mode, 
                    config.log_file[0] ? config.log_file : NULL) < 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return EXIT_FAILURE;
    }
    
    // Setup signal handlers
    if (install_signal_handlers() < 0) {
        log_error("Failed to install signal handlers");
        return EXIT_FAILURE;
    }
    
    // Create PID file
    if (create_pid_file(config.pid_file) < 0) {
        log_error("Failed to create PID file");
        return EXIT_FAILURE;
    }
    
    // Daemonize if requested
    if (config.daemon_mode) {
        if (daemonize() < 0) {
            log_error("Failed to daemonize");
            remove_pid_file(config.pid_file);
            return EXIT_FAILURE;
        }
    }
    
    // Setup firewall rules
    if (setup_firewall_rules() < 0) {
        log_error("Failed to setup firewall rules");
        remove_pid_file(config.pid_file);
        return EXIT_FAILURE;
    }
    
    // Initialize netfilter queue
    if (netfilter_init(&nfq_ctx, 0, packet_process_callback) < 0) {
        log_error("Failed to initialize netfilter queue");
        cleanup_firewall_rules();
        remove_pid_file(config.pid_file);
        return EXIT_FAILURE;
    }
    
    log_info("GoodbyeDPI started successfully");
    log_info("Main loop started - processing packets");
    
    // Main packet processing loop
    while (is_running()) {
        if (netfilter_receive_packet(&nfq_ctx) < 0) {
            log_error("Error receiving packet");
            if (!is_running()) break;
            usleep(100000);  // Sleep 100ms on error
            continue;
        }
        
        // Print stats periodically
        if (packets_processed % 1000 == 0 && config.verbose_mode) {
            log_packet_stats(packets_processed, packets_modified, bytes_processed);
        }
    }
    
    log_info("Main loop ended");
    
    // Cleanup
    netfilter_cleanup(&nfq_ctx);
    cleanup_firewall_rules();
    remove_pid_file(config.pid_file);
    logging_cleanup();
    
    log_info("GoodbyeDPI stopped successfully");
    
    return EXIT_SUCCESS;
}