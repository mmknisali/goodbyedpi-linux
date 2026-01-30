#include "../include/goodbyedpi.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Simple hash function for strings (djb2 algorithm)
unsigned int hash_string(const char *str)
{
    unsigned long hash = 5381;
    int c;
    
    if (!str) return 0;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return (unsigned int)hash;
}

// Hash function for byte arrays
unsigned int hash_bytes(const uint8_t *data, size_t len)
{
    unsigned long hash = 5381;
    
    if (!data || len == 0) return 0;
    
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + data[i]; // hash * 33 + byte
    }
    
    return (unsigned int)hash;
}

// Simple hash for connection tracking (combines IP, port, protocol)
unsigned int hash_connection(uint32_t src_ip, uint32_t dst_ip, 
                           uint16_t src_port, uint16_t dst_port, 
                           uint8_t protocol)
{
    // Combine all connection parameters into a single hash
    unsigned long hash = 5381;
    
    hash = ((hash << 5) + hash) + (src_ip & 0xFF);
    hash = ((hash << 5) + hash) + ((src_ip >> 8) & 0xFF);
    hash = ((hash << 5) + hash) + ((src_ip >> 16) & 0xFF);
    hash = ((hash << 5) + hash) + ((src_ip >> 24) & 0xFF);
    
    hash = ((hash << 5) + hash) + (dst_ip & 0xFF);
    hash = ((hash << 5) + hash) + ((dst_ip >> 8) & 0xFF);
    hash = ((hash << 5) + hash) + ((dst_ip >> 16) & 0xFF);
    hash = ((hash << 5) + hash) + ((dst_ip >> 24) & 0xFF);
    
    hash = ((hash << 5) + hash) + (src_port & 0xFF);
    hash = ((hash << 5) + hash) + ((src_port >> 8) & 0xFF);
    
    hash = ((hash << 5) + hash) + (dst_port & 0xFF);
    hash = ((hash << 5) + hash) + ((dst_port >> 8) & 0xFF);
    
    hash = ((hash << 5) + hash) + protocol;
    
    return (unsigned int)hash;
}

// IPv6 connection hash
unsigned int hash_connection_ipv6(const uint32_t src_ip[4], const uint32_t dst_ip[4],
                                uint16_t src_port, uint16_t dst_port,
                                uint8_t protocol)
{
    unsigned long hash = 5381;
    
    for (int i = 0; i < 4; i++) {
        hash = ((hash << 5) + hash) + (src_ip[i] & 0xFF);
        hash = ((hash << 5) + hash) + ((src_ip[i] >> 8) & 0xFF);
        hash = ((hash << 5) + hash) + ((src_ip[i] >> 16) & 0xFF);
        hash = ((hash << 5) + hash) + ((src_ip[i] >> 24) & 0xFF);
        
        hash = ((hash << 5) + hash) + (dst_ip[i] & 0xFF);
        hash = ((hash << 5) + hash) + ((dst_ip[i] >> 8) & 0xFF);
        hash = ((hash << 5) + hash) + ((dst_ip[i] >> 16) & 0xFF);
        hash = ((hash << 5) + hash) + ((dst_ip[i] >> 24) & 0xFF);
    }
    
    hash = ((hash << 5) + hash) + (src_port & 0xFF);
    hash = ((hash << 5) + hash) + ((src_port >> 8) & 0xFF);
    
    hash = ((hash << 5) + hash) + (dst_port & 0xFF);
    hash = ((hash << 5) + hash) + ((dst_port >> 8) & 0xFF);
    
    hash = ((hash << 5) + hash) + protocol;
    
    return (unsigned int)hash;
}

// Simple checksum for data integrity
uint16_t hash_checksum(const uint8_t *data, size_t len)
{
    uint32_t sum = 0;
    
    if (!data || len == 0) return 0;
    
    // Calculate 16-bit sum
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    
    // Fold to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)~sum;
}

// Combined hash for packets (includes direction and basic packet info)
unsigned int hash_packet(const packet_t *packet)
{
    if (!packet) return 0;
    
    unsigned long hash = 5381;
    
    if (packet->is_ipv6) {
        hash = ((hash << 5) + hash) + hash_connection_ipv6(
            packet->src_ip, packet->dst_ip, 
            packet->src_port, packet->dst_port, 
            packet->is_outbound ? IPPROTO_TCP : IPPROTO_UDP);
    } else {
        hash = ((hash << 5) + hash) + hash_connection(
            packet->src_ip[0], packet->dst_ip[0],
            packet->src_port, packet->dst_port,
            packet->is_outbound ? IPPROTO_TCP : IPPROTO_UDP);
    }
    
    hash = ((hash << 5) + hash) + packet->type;
    hash = ((hash << 5) + hash) + packet->direction;
    hash = ((hash << 5) + hash) + packet->ttl;
    
    return (unsigned int)hash;
}