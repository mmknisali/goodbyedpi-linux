#include <netinet/ip6.h>
#include <string.h>
#include "../include/goodbyedpi.h"
#include "../include/packet.h"
#include "../include/logging.h"
#include "../include/config.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>



// Initialize packet structure
int packet_init(packet_t *packet)
{
    if (!packet) {
        return -1;
    }
    
    memset(packet, 0, sizeof(packet_t));
    packet->type = PACKET_UNKNOWN;
    packet->direction = DIRECTION_UNKNOWN;
    
    return 0;
}

// Free packet resources
void packet_free(packet_t *packet)
{
    if (!packet) {
        return;
    }
    
    if (packet->payload) {
        free(packet->payload);
        packet->payload = NULL;
    }
    
    if (packet->headers) {
        free(packet->headers);
        packet->headers = NULL;
    }
    
    if (packet->raw_packet) {
        free(packet->raw_packet);
        packet->raw_packet = NULL;
    }
    
    packet_init(packet);
}

// Parse IPv4 packet
int packet_parse_ipv4(const uint8_t *data, size_t len, packet_t *packet)
{
    struct iphdr *ip_hdr;
    struct tcphdr *tcp_hdr;
    struct udphdr *udp_hdr;
    uint8_t *payload;
    size_t payload_len;
    
    if (!data || len < sizeof(struct iphdr)) {
        return -1;
    }
    
    ip_hdr = (struct iphdr *)data;
    
    if (ip_hdr->version != 4) {
        return -1;  // Not IPv4
    }
    
    // Initialize packet structure
    packet_init(packet);
    packet->is_ipv6 = false;
    packet->direction = DIRECTION_UNKNOWN;
    packet->ttl = ip_hdr->ttl;
    
    // Extract IP addresses
    packet->src_ip[0] = ip_hdr->saddr;
    packet->dst_ip[0] = ip_hdr->daddr;
    
    // Set direction based on comparison with local interfaces (simplified)
    // In a real implementation, you'd check against actual interface IPs
    packet->is_outbound = (ntohl(ip_hdr->saddr) < ntohl(ip_hdr->daddr));
    
    // Handle transport layer
    if (ip_hdr->protocol == IPPROTO_TCP) {
        if (len < (sizeof(struct iphdr) + sizeof(struct tcphdr))) {
            return -1;
        }
        
        tcp_hdr = (struct tcphdr *)(data + ip_hdr->ihl * 4);
        packet->src_port = ntohs(tcp_hdr->source);
        packet->dst_port = ntohs(tcp_hdr->dest);
        
        payload = (uint8_t *)tcp_hdr + tcp_hdr->doff * 4;
        payload_len = len - ip_hdr->ihl * 4 - tcp_hdr->doff * 4;
        
        if (payload_len > 0) {
            packet->type = PACKET_IPV4_TCP_DATA;
            packet->payload = malloc(payload_len);
            if (packet->payload) {
                memcpy(packet->payload, payload, payload_len);
                packet->payload_len = payload_len;
            }
        } else {
            packet->type = PACKET_IPV4_TCP;
        }
    }
    else if (ip_hdr->protocol == IPPROTO_UDP) {
        if (len < (sizeof(struct iphdr) + sizeof(struct udphdr))) {
            return -1;
        }
        
        udp_hdr = (struct udphdr *)(data + ip_hdr->ihl * 4);
        packet->src_port = ntohs(udp_hdr->source);
        packet->dst_port = ntohs(udp_hdr->dest);
        
        payload = (uint8_t *)udp_hdr + sizeof(struct udphdr);
        payload_len = len - ip_hdr->ihl * 4 - sizeof(struct udphdr);
        
        if (payload_len > 0) {
            packet->type = PACKET_IPV4_UDP_DATA;
            packet->payload = malloc(payload_len);
            if (packet->payload) {
                memcpy(packet->payload, payload, payload_len);
                packet->payload_len = payload_len;
            }
        }
    }
    
    // Store raw packet data for reinjection
    packet->raw_packet_len = len;
    packet->raw_packet = malloc(len);
    if (packet->raw_packet) {
        memcpy(packet->raw_packet, data, len);
    }
    
    // Store headers
    size_t headers_len = ip_hdr->ihl * 4;
    if (ip_hdr->protocol == IPPROTO_TCP) {
        struct tcphdr *tcp_hdr = (struct tcphdr *)(data + headers_len);
        headers_len += tcp_hdr->doff * 4;
    } else if (ip_hdr->protocol == IPPROTO_UDP) {
        headers_len += sizeof(struct udphdr);
    }
    
    packet->headers_len = headers_len;
    packet->headers = malloc(headers_len);
    if (packet->headers) {
        memcpy(packet->headers, data, headers_len);
    }
    
    return 0;
}

// Parse IPv6 packet (placeholder - would need full implementation)
int packet_parse_ipv6(const uint8_t *data, size_t len, packet_t *packet)
{
    // Simplified IPv6 parsing - would need full implementation
    packet_init(packet);
    packet->is_ipv6 = true;
    packet->direction = DIRECTION_UNKNOWN;
    packet->type = PACKET_IPV6_TCP;  // Simplified
    
    // Store raw packet data
    packet->raw_packet_len = len;
    packet->raw_packet = malloc(len);
    if (packet->raw_packet) {
        memcpy(packet->raw_packet, data, len);
    }
    
    return 0;
}

// Main packet parsing function
int packet_parse(const uint8_t *data, size_t len, packet_t *packet)
{
    if (!data || !packet || len < sizeof(struct iphdr)) {
        return -1;
    }
    
    // Check IP version
    uint8_t version = (data[0] >> 4) & 0x0F;
    
    if (version == 4) {
        return packet_parse_ipv4(data, len, packet);
    } else if (version == 6) {
        return packet_parse_ipv6(data, len, packet);
    }
    
    return -1;  // Unsupported protocol
}

// Check if packet is TCP
bool packet_is_tcp(const packet_t *packet)
{
    if (!packet) {
        return 0;
    }
    
    return (packet->type == PACKET_IPV4_TCP || 
            packet->type == PACKET_IPV4_TCP_DATA ||
            packet->type == PACKET_IPV6_TCP || 
            packet->type == PACKET_IPV6_TCP_DATA);
}

// Check if packet is UDP
bool packet_is_udp(const packet_t *packet)
{
    if (!packet) {
        return false;
    }
    
    return (packet->type == PACKET_IPV4_UDP_DATA || 
            packet->type == PACKET_IPV6_UDP_DATA);
}

// Check if packet is HTTP
bool packet_is_http(const packet_t *packet)
{
    if (!packet || !packet_is_tcp(packet) || !packet->payload) {
        return false;
    }
    
    // Check for common HTTP methods
    return (packet->dst_port == 80 && 
            (packet->payload_len >= 3 &&
             (strncmp((char*)packet->payload, "GET", 3) == 0 ||
              strncmp((char*)packet->payload, "POST", 4) == 0 ||
              strncmp((char*)packet->payload, "HEAD", 4) == 0 ||
              strncmp((char*)packet->payload, "PUT", 3) == 0 ||
              strncmp((char*)packet->payload, "DELETE", 6) == 0)));
}

// Check if packet is HTTPS
bool packet_is_https(const packet_t *packet)
{
    if (!packet || !packet_is_tcp(packet)) {
        return false;
    }
    
    return (packet->dst_port == 443);
}

// Copy packet structure
int packet_copy(const packet_t *src, packet_t *dst)
{
    if (!src || !dst) {
        return -1;
    }
    
    packet_init(dst);
    
    dst->is_ipv6 = src->is_ipv6;
    dst->is_outbound = src->is_outbound;
    dst->type = src->type;
    dst->direction = src->direction;
    dst->ttl = src->ttl;
    dst->src_port = src->src_port;
    dst->dst_port = src->dst_port;
    dst->nfqueue_id = src->nfqueue_id;
    
    memcpy(dst->src_ip, src->src_ip, sizeof(dst->src_ip));
    memcpy(dst->dst_ip, src->dst_ip, sizeof(dst->dst_ip));
    
    if (src->payload && src->payload_len > 0) {
        dst->payload = malloc(src->payload_len);
        if (dst->payload) {
            memcpy(dst->payload, src->payload, src->payload_len);
            dst->payload_len = src->payload_len;
        }
    }
    
    if (src->headers && src->headers_len > 0) {
        dst->headers = malloc(src->headers_len);
        if (dst->headers) {
            memcpy(dst->headers, src->headers, src->headers_len);
            dst->headers_len = src->headers_len;
        }
    }
    
    if (src->raw_packet && src->raw_packet_len > 0) {
        dst->raw_packet = malloc(src->raw_packet_len);
        if (dst->raw_packet) {
            memcpy(dst->raw_packet, src->raw_packet, src->raw_packet_len);
            dst->raw_packet_len = src->raw_packet_len;
        }
    }
    
    return 0;
}

// Print packet information (for debugging)
void print_packet_info(const packet_t *packet)
{
    if (!packet) {
        log_debug("NULL packet");
        return;
    }
    
    char src_ip[INET6_ADDRSTRLEN] = {0};
    char dst_ip[INET6_ADDRSTRLEN] = {0};
    
    if (!packet->is_ipv6) {
        inet_ntop(AF_INET, &packet->src_ip[0], src_ip, sizeof(src_ip));
        inet_ntop(AF_INET, &packet->dst_ip[0], dst_ip, sizeof(dst_ip));
    } else {
        inet_ntop(AF_INET6, packet->src_ip, src_ip, sizeof(src_ip));
        inet_ntop(AF_INET6, packet->dst_ip, dst_ip, sizeof(dst_ip));
    }
    
    log_debug("Packet: %s %s:%u -> %s:%u (type=%d, ttl=%u, payload=%zu bytes)",
              packet->is_outbound ? "OUT" : "IN",
              src_ip, packet->src_port,
              dst_ip, packet->dst_port,
              packet->type, packet->ttl, packet->payload_len);
}
int packet_set_ttl(packet_t *packet, uint8_t ttl) {
    if (!packet || !packet->raw_packet) {
        return -1;
    }

    // Update the struct's internal TTL field
    packet->ttl = ttl;

    if (packet->type == PACKET_IPV4_TCP) {
        struct iphdr *iph = (struct iphdr *)packet->raw_packet;
        iph->ttl = ttl;
        
        // Use the project's internal checksum update
        return 0;
    } 
    else if (packet->type == PACKET_IPV6_TCP) {
        struct ip6_hdr *ip6h = (struct ip6_hdr *)packet->raw_packet;
        ip6h->ip6_hlim = ttl;
        return 0;
    }

    return -1;
}
