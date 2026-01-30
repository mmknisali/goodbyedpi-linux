#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include "../include/config.h"
#include <time.h>

// Connection tracking table (simplified)
#define MAX_CONNECTIONS 4096

typedef struct {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t ttl;
    uint8_t protocol;
    time_t last_seen;
    bool valid;
} conntrack_entry_t;

static conntrack_entry_t conntrack_table[MAX_CONNECTIONS];
static int conntrack_initialized = 0;

// Initialize connection tracking
int conntrack_init(void)
{
    memset(conntrack_table, 0, sizeof(conntrack_table));
    conntrack_initialized = 1;
    log_info("Connection tracking initialized");
    return 0;
}

// Cleanup connection tracking
int conntrack_cleanup(void)
{
    memset(conntrack_table, 0, sizeof(conntrack_table));
    conntrack_initialized = 0;
    log_info("Connection tracking cleaned up");
    return 0;
}

// Add connection to tracking table
int conntrack_add(const packet_t *packet)
{
    if (!packet || !conntrack_initialized) {
        return -1;
    }
    
    uint32_t hash = (packet->src_ip[0] + packet->dst_ip[0] + 
                     packet->src_port + packet->dst_port) % MAX_CONNECTIONS;
    
    conntrack_entry_t *entry = &conntrack_table[hash];
    
    // Initialize entry
    memset(entry, 0, sizeof(conntrack_entry_t));
    entry->valid = true;
    entry->src_addr = packet->src_ip[0];
    entry->dst_addr = packet->dst_ip[0];
    entry->src_port = packet->src_port;
    entry->dst_port = packet->dst_port;
    entry->ttl = packet->ttl;
    entry->protocol = packet_is_tcp(packet) ? IPPROTO_TCP : IPPROTO_UDP;
    entry->last_seen = time(NULL);
    
    log_debug("Added connection tracking entry");
    return 0;
}

// Find connection entry
static conntrack_entry_t *find_conntrack_entry(const packet_t *packet)
{
    if (!packet || !conntrack_initialized) {
        return NULL;
    }
    
    uint32_t hash = (packet->src_ip[0] + packet->dst_ip[0] + 
                     packet->src_port + packet->dst_port) % MAX_CONNECTIONS;
    
    conntrack_entry_t *entry = &conntrack_table[hash];
    
    if (!entry->valid) {
        return NULL;
    }
    
    // Check if this is same connection
    if (entry->src_addr == packet->src_ip[0] &&
        entry->dst_addr == packet->dst_ip[0] &&
        entry->src_port == packet->src_port &&
        entry->dst_port == packet->dst_port) {
        
        return entry;
    }
    
    return NULL;
}

// Lookup connection entry
int conntrack_lookup(const packet_t *packet, conntrack_info_t *info)
{
    if (!packet || !info || !conntrack_initialized) {
        return -1;
    }
    
    conntrack_entry_t *entry = find_conntrack_entry(packet);
    
    if (!entry) {
        return -1;
    }
    
    // Update last seen time
    entry->last_seen = time(NULL);
    
    // Copy tracking information
    info->valid = true;
    info->src_addr[0] = entry->src_addr;
    info->dst_addr[0] = entry->dst_addr;
    info->src_port = entry->src_port;
    info->dst_port = entry->dst_port;
    info->ttl = entry->ttl;
    info->protocol = entry->protocol;
    info->last_seen = entry->last_seen;
    
    log_debug("Found connection tracking entry");
    return 0;
}

// Cleanup old entries
int conntrack_cleanup_old(void)
{
    if (!conntrack_initialized) {
        return 0;
    }
    
    time_t now = time(NULL);
    int removed = 0;
    
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        conntrack_entry_t *entry = &conntrack_table[i];
        
        if (entry->valid && (now - entry->last_seen > 300)) {
            // Remove old entry
            memset(entry, 0, sizeof(conntrack_entry_t));
            removed++;
        }
    }
    
    if (removed > 0) {
        log_debug("Cleaned up %d old connection entries", removed);
    }
    
    return removed;
}