#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <string.h>
#include <stdlib.h>

// TLS record structure
typedef struct {
    uint8_t type;
    uint16_t version;
    uint16_t length;
} tls_record_header_t;

// TLS handshake structure
typedef struct {
    uint8_t type;
    uint8_t length[3]; // 24-bit length
    uint16_t version;
} tls_handshake_header_t;

// Extract SNI from TLS ClientHello
int evasion_extract_sni(const uint8_t *tls_data, size_t tls_len, char *hostname, size_t hostname_len)
{
    if (!tls_data || tls_len < 5 || !hostname || hostname_len == 0) {
        return -1;
    }
    
    // Skip TCP header (assuming tls_data starts at TLS record)
    const uint8_t *data = tls_data;
    size_t data_len = tls_len;
    
    // Parse TLS record header
    if (data_len < sizeof(tls_record_header_t)) {
        return -1;
    }
    
    const tls_record_header_t *record = (const tls_record_header_t *)data;
    
    // Check if this's a handshake record
    if (record->type != 0x16) { // Handshake type
        return -1;
    }
    
    // Check TLS version
    if (record->version != 0x0301 && record->version != 0x0303) { // TLS 1.0/1.1/1.2
        return -1;
    }
    
    // Move to handshake data
    data += sizeof(tls_record_header_t);
    data_len -= sizeof(tls_record_header_t);
    
    // Check if we have enough data for handshake header
    if (data_len < sizeof(tls_handshake_header_t)) {
        return -1;
    }
    
    const tls_handshake_header_t *handshake = (const tls_handshake_header_t *)data;
    
    // Check if this's a ClientHello
    if (handshake->type != 0x01) { // ClientHello
        return -1;
    }
    
    // Calculate handshake length (24-bit)
    uint32_t handshake_len = (handshake->length[0] << 16) | 
                            (handshake->length[1] << 8) | 
                            handshake->length[2];
    
    // Move to ClientHello data
    data += sizeof(tls_handshake_header_t);
    data_len -= sizeof(tls_handshake_header_t);
    
    if (data_len < handshake_len) {
        return -1;
    }
    
    // Skip session ID (1 byte length + session ID)
    if (data_len < 1) return -1;
    uint8_t session_id_len = data[0];
    data += 1 + session_id_len;
    data_len -= 1 + session_id_len;
    if (data_len < 2) return -1;
    
    // Skip cipher suites (2 bytes length + cipher suites)
    uint16_t cipher_suites_len = (data[0] << 8) | data[1];
    data += 2 + cipher_suites_len;
    data_len -= 2 + cipher_suites_len;
    if (data_len < 1) return -1;
    
    // Skip compression methods (1 byte length + methods)
    uint8_t compression_methods_len = data[0];
    data += 1 + compression_methods_len;
    data_len -= 1 + compression_methods_len;
    if (data_len < 2) return -1;
    
    // Parse extensions
    uint16_t extensions_len = (data[0] << 8) | data[1];
    data += 2;
    data_len -= 2;
    
    if (data_len < extensions_len) {
        return -1;
    }
    
    // Iterate through extensions to find SNI
    const uint8_t *extensions_end = data + extensions_len;
    while (data < extensions_end) {
        if (data + 4 > extensions_end) break;
        
        uint16_t ext_type = (data[0] << 8) | data[1];
        uint16_t ext_len = (data[2] << 8) | data[3];
        data += 4;
        
        if (data + ext_len > extensions_end) break;
        
        if (ext_type == 0x0000) { // SNI extension
            return parse_sni_extension(data, ext_len, hostname, hostname_len);
        }
        
        data += ext_len;
    }
    
    return -1; // SNI not found
}

// Parse SNI extension
int parse_sni_extension(const uint8_t *ext_data, size_t ext_len, char *hostname, size_t hostname_len)
{
    if (!ext_data || ext_len < 5 || !hostname || hostname_len == 0) {
        return -1;
    }
    
    // Skip SNI extension length (2 bytes)
    const uint8_t *data = ext_data + 2;
    size_t data_len = ext_len - 2;
    
    if (data_len < 3) return -1;
    
    // Check name type (should be 0 for hostname)
    if (data[0] != 0x00) {
        return -1;
    }
    
    // Get hostname length
    uint16_t name_len = (data[1] << 8) | data[2];
    data += 3;
    data_len -= 3;
    
    if (data_len < name_len || name_len >= hostname_len) {
        return -1;
    }
    
    // Copy hostname
    memcpy(hostname, data, name_len);
    hostname[name_len] = '\0';
    
    log_debug("Extracted SNI: %s", hostname);
    return 0;
}

// Check if hostname should be processed based on SNI
int should_process_by_sni(const packet_t *packet, char *hostname, size_t hostname_len)
{
    if (!packet || !hostname) {
        return 0;
    }
    
    // Only process HTTPS traffic with TLS data
    if (packet->type != PACKET_IPV4_TCP_DATA && packet->type != PACKET_IPV6_TCP_DATA) {
        return 0;
    }
    
    if ((packet->dst_port != 443 && packet->src_port != 443) || !packet->payload) {
        return 0;
    }
    
    // Extract SNI
    if (evasion_extract_sni(packet->payload, packet->payload_len, hostname, hostname_len) != 0) {
        return 0; // No SNI found
    }
    
    // Check if hostname is in blacklist
    if (config.enable_blacklist && config.blacklist_file[0] != '\0') {
        // TODO: Implement blacklist check
        log_debug("Blacklist check not implemented yet");
    }
    
    // Check if hostname is in whitelist
    if (config.enable_whitelist && config.whitelist_file[0] != '\0') {
        // TODO: Implement whitelist check
        log_debug("Whitelist check not implemented yet");
    }
    
    // Check for SNI-based fragmentation
    if (config.fragment_by_sni) {
        // TODO: Implement SNI-based fragmentation logic
        log_debug("SNI-based fragmentation not implemented yet");
    }
    
    return 1; // Process this packet
}

// Extract SNI from packet and return hostname
char *extract_sni_from_packet(const packet_t *packet)
{
    if (!packet || !packet->payload || packet->payload_len < 5) {
        return NULL;
    }
    
    char *hostname = malloc(MAX_HOSTNAME_LEN);
    if (!hostname) {
        return NULL;
    }
    
    if (evasion_extract_sni(packet->payload, packet->payload_len, hostname, MAX_HOSTNAME_LEN) == 0) {
        return hostname;
    }
    
    free(hostname);
    return NULL;
}