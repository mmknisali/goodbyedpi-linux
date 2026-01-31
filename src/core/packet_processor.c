#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include "../include/config.h"
#include "../include/packet.h"

// Helper function to find Host header in HTTP payload
static char *find_host_header(uint8_t *payload, size_t payload_len)
{
    if (!payload || payload_len < 6) return NULL;
    
    char *payload_str = (char *)payload;
    char *host_pos = stristr(payload_str, "Host:");
    
    return host_pos;
}

// Helper function to mix case in Host header value
static int apply_host_mixedcase(uint8_t *payload, size_t payload_len)
{
    char *host_pos = find_host_header(payload, payload_len);
    if (!host_pos) return -1;
    
    // Find start of host value (after "Host:")
    char *value_start = host_pos + 5;
    
    // Skip whitespace
    while (*value_start && (*value_start == ' ' || *value_start == '\t')) {
        value_start++;
    }
    
    // Find end of line
    char *value_end = value_start;
    while (*value_end && *value_end != '\r' && *value_end != '\n') {
        value_end++;
    }
    
    size_t value_len = value_end - value_start;
    if (value_len > 0) {
        mix_case(value_start, value_len);
        log_debug("Applied mixed case to Host header value");
        return 0;
    }
    
    return -1;
}

// Helper function to remove space after Host:
static int apply_host_removespace(uint8_t *payload, size_t payload_len)
{
    char *host_pos = find_host_header(payload, payload_len);
    if (!host_pos) return -1;
    
    // Check if there's a space after "Host:"
    char *after_colon = host_pos + 5;
    if (*after_colon == ' ') {
        // Shift the rest left by one character
        memmove(after_colon, after_colon + 1, strlen(after_colon + 1) + 1);
        log_debug("Removed space after Host:");
        return 0;
    }
    
    return -1;
}

// Helper function to add additional space
static int apply_additional_space(uint8_t *payload, size_t payload_len)
{
    (void)payload_len;
    char *host_pos = find_host_header(payload, payload_len);
    if (!host_pos) return -1;
    
    // Add extra space after "Host:" by shifting right
    char *insert_pos = host_pos + 5;
    size_t remaining_len = strlen(insert_pos) + 1;
    
    // Make sure we have room (this is simplified - in production would need buffer management)
    if (remaining_len > 0) {
        memmove(insert_pos + 1, insert_pos, remaining_len);
        *insert_pos = ' ';
        log_debug("Added additional space after Host:");
        return 0;
    }
    
    return -1;
}

// Core packet processing function
int packet_process(packet_t *packet)
{
    if (!packet) {
        log_error("NULL packet received");
        return -1;
    }
    
    log_debug("Processing packet: type=%d, is_ipv6=%d, outbound=%d",
              packet->type, packet->is_ipv6, packet->is_outbound);
    
    // Apply evasion techniques based on configuration
    
    // Skip packets that are too large
    if (config.max_payload_size > 0 && 
        packet->payload_len > config.max_payload_size) {
        log_debug("Skipping large packet: payload size=%zu", packet->payload_len);
        return 0;
    }
    
    int modified = 0;
    
    // HTTP packet processing
    if (packet_is_http(packet)) {
        log_debug("Processing HTTP packet");
        
        if (config.host_mixedcase && packet->payload) {
            if (apply_host_mixedcase(packet->payload, packet->payload_len) == 0) {
                modified = 1;
            }
        }
        
        if (config.host_removespace && packet->payload) {
            if (apply_host_removespace(packet->payload, packet->payload_len) == 0) {
                modified = 1;
            }
        }
        
        if (config.additional_space && packet->payload) {
            if (apply_additional_space(packet->payload, packet->payload_len) == 0) {
                modified = 1;
            }
        }
        
        // HTTP fragmentation
        if (config.http_fragment_size > 0) {
            if (evasion_fragment_packet(packet, config.http_fragment_size) == 0) {
                modified = 1;
            }
        }
    }
    
    // HTTPS packet processing
    else if (packet_is_https(packet)) {
        log_debug("Processing HTTPS packet");
        
        // HTTPS fragmentation
        if (config.https_fragment_size > 0) {
            if (evasion_fragment_packet(packet, config.https_fragment_size) == 0) {
                modified = 1;
            }
        }
        
        // Fake packet injection
        if (config.fake_packet) {
            if (evasion_inject_fake_packet(packet) == 0) {
                modified = 1;
            }
        }
    }
    
    log_debug("Packet processing completed: modified=%d", modified);
    return modified;
}