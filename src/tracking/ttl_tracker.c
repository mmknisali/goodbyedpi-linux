#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <string.h>
#include <time.h>

// TTL tracking structure
typedef struct ttl_track_entry {
    uint32_t src_ip[4];
    uint32_t dst_ip[4];
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    uint8_t original_ttl;
    uint8_t min_ttl;
    uint8_t max_ttl;
    uint8_t ttl_delta;
    time_t first_seen;
    time_t last_seen;
    uint32_t packet_count;
    bool is_established;
} ttl_track_entry_t;

// TTL tracking table (simplified - in production would use hash table)
#define MAX_TTL_ENTRIES 1024
static ttl_track_entry_t ttl_table[MAX_TTL_ENTRIES];
static size_t ttl_table_size = 0;

// TTL learning parameters
#define TTL_LEARNING_PACKETS 10
#define TTL_LEARNING_TIMEOUT 30
#define TTL_ESTABLISHED_TIMEOUT 300
#define TTL_MAX_DELTA 5

// Initialize TTL tracking
int ttl_tracker_init(void)
{
    memset(ttl_table, 0, sizeof(ttl_table));
    ttl_table_size = 0;
    log_info("TTL tracker initialized");
    return 0;
}

// Cleanup TTL tracking
int ttl_tracker_cleanup(void)
{
    ttl_table_size = 0;
    memset(ttl_table, 0, sizeof(ttl_table));
    log_info("TTL tracker cleaned up");
    return 0;
}

// Find TTL entry for connection
static ttl_track_entry_t *find_ttl_entry(const packet_t *packet)
{
    for (size_t i = 0; i < ttl_table_size; i++) {
        ttl_track_entry_t *entry = &ttl_table[i];
        
        if (entry->protocol == 0) continue; // Empty entry
        
        // Check for connection match (bidirectional)
        bool match = false;
        
        if (!packet->is_ipv6) {
            // IPv4
            match = (entry->src_ip[0] == packet->src_ip[0] &&
                    entry->dst_ip[0] == packet->dst_ip[0] &&
                    entry->src_port == packet->src_port &&
                    entry->dst_port == packet->dst_port &&
                    entry->protocol == IPPROTO_TCP);
                    
            if (!match) {
                // Try reverse direction
                match = (entry->src_ip[0] == packet->dst_ip[0] &&
                        entry->dst_ip[0] == packet->src_ip[0] &&
                        entry->src_port == packet->dst_port &&
                        entry->dst_port == packet->src_port &&
                        entry->protocol == IPPROTO_TCP);
            }
        } else {
            // IPv6
            match = (memcmp(entry->src_ip, packet->src_ip, sizeof(uint32_t) * 4) == 0 &&
                    memcmp(entry->dst_ip, packet->dst_ip, sizeof(uint32_t) * 4) == 0 &&
                    entry->src_port == packet->src_port &&
                    entry->dst_port == packet->dst_port &&
                    entry->protocol == IPPROTO_TCP);
                    
            if (!match) {
                // Try reverse direction
                match = (memcmp(entry->src_ip, packet->dst_ip, sizeof(uint32_t) * 4) == 0 &&
                        memcmp(entry->dst_ip, packet->src_ip, sizeof(uint32_t) * 4) == 0 &&
                        entry->src_port == packet->dst_port &&
                        entry->dst_port == packet->src_port &&
                        entry->protocol == IPPROTO_TCP);
            }
        }
        
        if (match) {
            return entry;
        }
    }
    
    return NULL;
}

// Add or update TTL entry
int ttl_track_update(const packet_t *packet, uint8_t ttl)
{
    if (!packet || packet->type != PACKET_IPV4_TCP && packet->type != PACKET_IPV6_TCP) {
        return -1; // Only track TCP connections
    }
    
    ttl_track_entry_t *entry = find_ttl_entry(packet);
    
    if (entry) {
        // Update existing entry
        entry->last_seen = time(NULL);
        entry->packet_count++;
        
        // Update TTL statistics if still in learning phase
        if (!entry->is_established) {
            if (ttl < entry->min_ttl || entry->min_ttl == 0) {
                entry->min_ttl = ttl;
            }
            if (ttl > entry->max_ttl || entry->max_ttl == 0) {
                entry->max_ttl = ttl;
            }
            
            // Calculate delta from original
            if (entry->original_ttl == 0) {
                entry->original_ttl = ttl;
            }
            
            entry->ttl_delta = entry->original_ttl - ttl;
            
            // Check if we have enough data to establish connection
            if (entry->packet_count >= TTL_LEARNING_PACKETS) {
                entry->is_established = true;
                log_debug("TTL tracking established: delta=%d, min=%d, max=%d", 
                         entry->ttl_delta, entry->min_ttl, entry->max_ttl);
            }
        }
        
        return 0;
    }
    
    // Create new entry
    if (ttl_table_size < MAX_TTL_ENTRIES) {
        entry = &ttl_table[ttl_table_size++];
    } else {
        // Find oldest non-established entry to replace
        entry = NULL;
        time_t oldest_time = time(NULL);
        
        for (size_t i = 0; i < ttl_table_size; i++) {
            if (!ttl_table[i].is_established && ttl_table[i].last_seen < oldest_time) {
                oldest_time = ttl_table[i].last_seen;
                entry = &ttl_table[i];
            }
        }
        
        if (!entry) {
            log_warning("TTL tracking table full");
            return -1;
        }
    }
    
    // Initialize new entry
    memset(entry, 0, sizeof(ttl_track_entry_t));
    
    if (!packet->is_ipv6) {
        entry->src_ip[0] = packet->src_ip[0];
        entry->dst_ip[0] = packet->dst_ip[0];
    } else {
        memcpy(entry->src_ip, packet->src_ip, sizeof(uint32_t) * 4);
        memcpy(entry->dst_ip, packet->dst_ip, sizeof(uint32_t) * 4);
    }
    
    entry->src_port = packet->src_port;
    entry->dst_port = packet->dst_port;
    entry->protocol = IPPROTO_TCP;
    entry->original_ttl = ttl;
    entry->min_ttl = ttl;
    entry->max_ttl = ttl;
    entry->first_seen = time(NULL);
    entry->last_seen = entry->first_seen;
    entry->packet_count = 1;
    entry->is_established = false;
    
    log_debug("Added TTL tracking entry: ttl=%d", ttl);
    return 0;
}

// Get auto TTL value for connection
uint8_t ttl_get_auto_ttl(uint8_t connection_ttl, uint8_t ttl_1, uint8_t ttl_2, 
                         uint8_t ttl_min, uint8_t ttl_max)
{
    // Calculate distance from server
    uint8_t distance = ttl_1 - connection_ttl;
    if (distance > ttl_2) {
        distance = ttl_2;
    }
    
    // Calculate fake TTL
    uint8_t fake_ttl = ttl_1 - distance;
    
    // Ensure TTL is within bounds
    if (fake_ttl < ttl_min) fake_ttl = ttl_min;
    if (fake_ttl > ttl_max) fake_ttl = ttl_max;
    
    return fake_ttl;
}

// Get TTL delta for established connection
int ttl_get_delta(const packet_t *packet)
{
    ttl_track_entry_t *entry = find_ttl_entry(packet);
    
    if (entry && entry->is_established) {
        return entry->ttl_delta;
    }
    
    return -1; // No established entry found
}

// Cleanup expired entries
int ttl_tracker_cleanup_expired(void)
{
    time_t now = time(NULL);
    int removed = 0;
    
    for (size_t i = 0; i < ttl_table_size; ) {
        ttl_track_entry_t *entry = &ttl_table[i];
        
        time_t timeout = entry->is_established ? TTL_ESTABLISHED_TIMEOUT : TTL_LEARNING_TIMEOUT;
        
        if (now - entry->last_seen > timeout) {
            // Remove entry by shifting remaining entries
            memmove(&ttl_table[i], &ttl_table[i + 1], 
                   (ttl_table_size - i - 1) * sizeof(ttl_track_entry_t));
            ttl_table_size--;
            removed++;
        } else {
            i++;
        }
    }
    
    if (removed > 0) {
        log_debug("Cleaned up %d expired TTL entries", removed);
    }
    
    return removed;
}

// Get TTL tracking statistics
void ttl_tracker_get_stats(size_t *total_entries, size_t *established_entries)
{
    if (total_entries) {
        *total_entries = ttl_table_size;
    }
    
    if (established_entries) {
        size_t established = 0;
        for (size_t i = 0; i < ttl_table_size; i++) {
            if (ttl_table[i].is_established) {
                established++;
            }
        }
        *established_entries = established;
    }
}

// Check if TTL tracking is ready for connection
bool ttl_is_ready(const packet_t *packet)
{
    ttl_track_entry_t *entry = find_ttl_entry(packet);
    return (entry && entry->is_established);
}