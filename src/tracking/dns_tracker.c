#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <string.h>
#include <time.h>
#include "../utils/uthash.h"

// DNS connection tracking structure
typedef struct dns_conntrack_entry {
    char key[128]; // Composite key
    dns_conntrack_info_t info;
    time_t last_seen;
    UT_hash_handle hh; // uthash handle
} dns_conntrack_entry_t;

// Global DNS tracking table
static dns_conntrack_entry_t *dns_table = NULL;

// DNS cleanup interval (seconds)
#define DNS_TRACKING_CLEANUP_INTERVAL 300

// DNS entry timeout (seconds)
#define DNS_TRACKING_TIMEOUT 60

// Initialize DNS tracking
int dns_tracker_init(void)
{
    dns_table = NULL;
    log_info("DNS tracker initialized");
    return 0;
}

// Cleanup DNS tracking
int dns_tracker_cleanup(void)
{
    dns_conntrack_entry_t *current, *tmp;
    
    HASH_ITER(hh, dns_table, current, tmp) {
        HASH_DEL(dns_table, current);
        free(current);
    }
    
    dns_table = NULL;
    log_info("DNS tracker cleaned up");
    return 0;
}

// Add DNS connection to tracking table
int dns_tracker_add(const packet_t *packet)
{
    if (!packet || packet->type != PACKET_IPV4_UDP_DATA || 
        (packet->src_port != 53 && packet->dst_port != 53)) {
        return -1; // Not a DNS packet
    }
    
    dns_conntrack_entry_t *entry = malloc(sizeof(dns_conntrack_entry_t));
    if (!entry) {
        log_error("Failed to allocate DNS tracking entry");
        return -1;
    }
    
    // Initialize entry
    memset(entry, 0, sizeof(dns_conntrack_entry_t));
    
    // Fill tracking information
    entry->info.valid = true;
    entry->info.timestamp = time(NULL);
    
    // Determine direction and set IPs/ports
    if (packet->src_port == 53) { // Response from DNS server
        memcpy(entry->info.dns_server_ip, packet->src_ip, sizeof(uint32_t) * 4);
        memcpy(entry->info.client_ip, packet->dst_ip, sizeof(uint32_t) * 4);
        entry->info.dns_server_port = packet->src_port;
        entry->info.client_port = packet->dst_port;
    } else { // Query to DNS server
        memcpy(entry->info.client_ip, packet->src_ip, sizeof(uint32_t) * 4);
        memcpy(entry->info.dns_server_ip, packet->dst_ip, sizeof(uint32_t) * 4);
        entry->info.client_port = packet->src_port;
        entry->info.dns_server_port = packet->dst_port;
    }
    
    entry->last_seen = entry->info.timestamp;
    
    // Create composite key
    snprintf(entry->key, sizeof(entry->key), 
             "%08x:%08x:%04x:%04x", 
             packet->src_ip[0], packet->dst_ip[0],
             packet->src_port, packet->dst_port);
    
    // Add to hash table
    HASH_ADD_STR(dns_table, key, entry);
    
    log_debug("Added DNS tracking entry: %s", entry->key);
    return 0;
}

// Lookup DNS connection in tracking table
int dns_tracker_lookup(const packet_t *packet, dns_conntrack_info_t *info)
{
    if (!packet || !info) {
        return -1;
    }
    
    if (packet->type != PACKET_IPV4_UDP_DATA || 
        (packet->src_port != 53 && packet->dst_port != 53)) {
        return -1; // Not a DNS packet
    }
    
    // Create lookup key
    char key[128];
    snprintf(key, sizeof(key), 
             "%08x:%08x:%04x:%04x", 
             packet->src_ip[0], packet->dst_ip[0],
             packet->src_port, packet->dst_port);
    
    // Try forward direction
    dns_conntrack_entry_t *entry = NULL;
    HASH_FIND_STR(dns_table, key, entry);
    
    if (!entry) {
        // Try reverse direction
        snprintf(key, sizeof(key), 
                 "%08x:%08x:%04x:%04x", 
                 packet->dst_ip[0], packet->src_ip[0],
                 packet->dst_port, packet->src_port);
        HASH_FIND_STR(dns_table, key, entry);
    }
    
    if (entry) {
        // Update last seen time
        entry->last_seen = time(NULL);
        
        // Copy tracking information
        memcpy(info, &entry->info, sizeof(dns_conntrack_info_t));
        
        log_debug("Found DNS tracking entry: %s", entry->key);
        return 0;
    }
    
    return -1; // Not found
}

// Update DNS connection timestamp
int dns_tracker_update(const packet_t *packet)
{
    dns_conntrack_info_t info;
    if (dns_tracker_lookup(packet, &info) == 0) {
        return 0; // Entry found and updated
    }
    
    // If not found, try to add it
    return dns_tracker_add(packet);
}

// Remove expired DNS entries
int dns_tracker_cleanup_expired(void)
{
    time_t now = time(NULL);
    dns_conntrack_entry_t *current, *tmp;
    int removed = 0;
    
    HASH_ITER(hh, dns_table, current, tmp) {
        if (now - current->last_seen > DNS_TRACKING_TIMEOUT) {
            log_debug("Removing expired DNS entry: %s", current->key);
            HASH_DEL(dns_table, current);
            free(current);
            removed++;
        }
    }
    
    if (removed > 0) {
        log_debug("Cleaned up %d expired DNS entries", removed);
    }
    
    return removed;
}

// Get DNS tracking statistics
void dns_tracker_get_stats(size_t *total_entries, size_t *active_entries)
{
    if (total_entries) {
        *total_entries = HASH_COUNT(dns_table);
    }
    
    if (active_entries) {
        time_t now = time(NULL);
        dns_conntrack_entry_t *current, *tmp;
        size_t active = 0;
        
        HASH_ITER(hh, dns_table, current, tmp) {
            if (now - current->last_seen <= DNS_TRACKING_TIMEOUT) {
                active++;
            }
        }
        *active_entries = active;
    }
}

// Check if DNS traffic should be redirected
int dns_should_redirect(const packet_t *packet)
{
    if (!packet) {
        return 0;
    }
    
    // Only redirect DNS queries (port 53)
    if (packet->type != PACKET_IPV4_UDP_DATA || packet->dst_port != 53) {
        return 0;
    }
    
    // Check if DNS redirection is enabled
    if (!config.dns_redirect_ipv4) {
        return 0;
    }
    
    // Could add additional checks here (e.g., specific DNS servers)
    
    return 1;
}

// Get configured DNS server
int dns_get_configured_server(uint32_t *server_ip, uint16_t *server_port)
{
    if (!server_ip || !server_port) {
        return -1;
    }
    
    if (config.dns_server_v4[0] == '\0') {
        return -1;
    }
    
    // Parse DNS server IP
    if (parse_ipv4_address(config.dns_server_v4, server_ip) != 0) {
        return -1;
    }
    
    *server_port = config.dns_port_v4;
    return 0;
}