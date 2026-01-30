#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <string.h>
#include <stdlib.h>

// Create fake TCP reset packet
int create_fake_reset_packet(const packet_t *original_packet, uint8_t **fake_packet, size_t *fake_packet_len)
{
    if (!original_packet || !fake_packet || !fake_packet_len) {
        return -1;
    }
    
    // Allocate space for fake packet
    *fake_packet_len = original_packet->headers_len;
    *fake_packet = malloc(*fake_packet_len);
    if (!*fake_packet) {
        log_error("Failed to allocate memory for fake packet");
        return -1;
    }
    
    // Copy original headers
    memcpy(*fake_packet, original_packet->headers, original_packet->headers_len);
    
    // Modify TCP header to set RST flag
    // This is a simplified implementation
    // Real implementation would need to parse and modify TCP header properly
    
    log_debug("Created fake reset packet: %zu bytes", *fake_packet_len);
    return 0;
}

// Create fake HTTP response
int create_fake_http_response(const packet_t *original_packet, uint8_t **fake_packet, size_t *fake_packet_len)
{
    if (!original_packet || !fake_packet || !fake_packet_len) {
        return -1;
    }
    
    const char *fake_response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    size_t response_len = strlen(fake_response);
    
    // Allocate space for fake packet (headers + response)
    *fake_packet_len = original_packet->headers_len + response_len;
    *fake_packet = malloc(*fake_packet_len);
    if (!*fake_packet) {
        log_error("Failed to allocate memory for fake HTTP response");
        return -1;
    }
    
    // Copy original headers
    memcpy(*fake_packet, original_packet->headers, original_packet->headers_len);
    
    // Add fake HTTP response
    memcpy(*fake_packet + original_packet->headers_len, fake_response, response_len);
    
    log_debug("Created fake HTTP response: %zu bytes", *fake_packet_len);
    return 0;
}

// Create fake DNS response
int create_fake_dns_response(const packet_t *original_packet, uint8_t **fake_packet, size_t *fake_packet_len)
{
    if (!original_packet || !fake_packet || !fake_packet_len) {
        return -1;
    }
    
    // This would create a fake DNS response
    // For now, just a placeholder implementation
    *fake_packet_len = original_packet->headers_len + 64; // Approximate DNS response size
    *fake_packet = malloc(*fake_packet_len);
    if (!*fake_packet) {
        log_error("Failed to allocate memory for fake DNS response");
        return -1;
    }
    
    // Copy original headers
    memcpy(*fake_packet, original_packet->headers, original_packet->headers_len);
    
    // Add fake DNS response data (simplified)
    uint8_t fake_dns[64] = {0};
    // Set DNS response flags (simplified)
    fake_dns[2] = 0x81; // Response with recursion available
    fake_dns[3] = 0x80; // Response code
    
    memcpy(*fake_packet + original_packet->headers_len, fake_dns, 64);
    
    log_debug("Created fake DNS response: %zu bytes", *fake_packet_len);
    return 0;
}

// Inject fake packet into network
int evasion_inject_fake_packet(const packet_t *original_packet)
{
    uint8_t *fake_packet = NULL;
    size_t fake_packet_len = 0;
    int result = -1;
    
    if (!original_packet) {
        return -1;
    }
    
    // Determine type of fake packet to create based on original packet
    if (original_packet->type == PACKET_IPV4_TCP || original_packet->type == PACKET_IPV6_TCP) {
        if (config.fake_packet) {
            result = create_fake_reset_packet(original_packet, &fake_packet, &fake_packet_len);
        }
    } else if (original_packet->type == PACKET_IPV4_UDP_DATA || original_packet->type == PACKET_IPV6_UDP_DATA) {
        if (original_packet->src_port == 53 || original_packet->dst_port == 53) {
            result = create_fake_dns_response(original_packet, &fake_packet, &fake_packet_len);
        }
    }
    
    if (result == 0 && fake_packet && fake_packet_len > 0) {
        // Send fake packet using raw socket
        result = send_raw_packet(fake_packet, fake_packet_len, original_packet->is_ipv6);
        
        if (result == 0) {
            log_info("Successfully injected fake packet");
        } else {
            log_error("Failed to inject fake packet");
        }
    }
    
    // Cleanup
    if (fake_packet) {
        free(fake_packet);
    }
    
    return result;
}

// Send fake packet with specific TTL
int send_fake_packet_with_ttl(const packet_t *original_packet, uint8_t ttl)
{
    uint8_t *fake_packet = NULL;
    size_t fake_packet_len = 0;
    int result = -1;
    
    if (!original_packet) {
        return -1;
    }
    
    result = create_fake_reset_packet(original_packet, &fake_packet, &fake_packet_len);
    if (result != 0) {
        return result;
    }
    
    // Modify TTL in IP header (simplified)
    // Real implementation would need to parse IP header properly
    
    // Send packet
    result = send_raw_packet(fake_packet, fake_packet_len, original_packet->is_ipv6);
    
    if (result == 0) {
        log_debug("Sent fake packet with TTL: %d", ttl);
    }
    
    free(fake_packet);
    return result;
}