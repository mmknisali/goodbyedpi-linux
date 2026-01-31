#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <string.h>
#include <errno.h>

// Convert IPv4 address string to binary format
int parse_ipv4_address(const char *ip_str, uint32_t *ip_addr)
{
    if (!ip_str || !ip_addr) return -1;
    
    struct in_addr addr;
    if (inet_pton(AF_INET, ip_str, &addr) != 1) {
        return -1;
    }
    
    *ip_addr = ntohl(addr.s_addr); // Convert to host byte order
    return 0;
}

// Convert IPv6 address string to binary format
int parse_ipv6_address(const char *ip_str, uint32_t ip_addr[4])
{
    if (!ip_str || !ip_addr) return -1;
    
    struct in6_addr addr;
    if (inet_pton(AF_INET6, ip_str, &addr) != 1) {
        return -1;
    }
    
    // Copy IPv6 address (stored in network byte order)
    memcpy(ip_addr, addr.s6_addr32, sizeof(uint32_t) * 4);
    return 0;
}

// Convert binary IPv4 address to string
int ipv4_address_to_string(uint32_t ip_addr, char *ip_str, size_t str_len)
{
    if (!ip_str || str_len < INET_ADDRSTRLEN) return -1;
    
    struct in_addr addr;
    addr.s_addr = htonl(ip_addr); // Convert to network byte order
    
    if (!inet_ntop(AF_INET, &addr, ip_str, str_len)) {
        return -1;
    }
    
    return 0;
}

// Convert binary IPv6 address to string
int ipv6_address_to_string(const uint32_t ip_addr[4], char *ip_str, size_t str_len)
{
    if (!ip_str || str_len < INET6_ADDRSTRLEN) return -1;
    
    struct in6_addr addr;
    memcpy(addr.s6_addr32, ip_addr, sizeof(uint32_t) * 4);
    
    if (!inet_ntop(AF_INET6, &addr, ip_str, str_len)) {
        return -1;
    }
    
    return 0;
}

// Check if IP address is in private range (RFC1918)
bool is_private_ipv4(uint32_t ip_addr)
{
    // 10.0.0.0/8
    if ((ip_addr & 0xFF000000) == 0x0A000000) return true;
    
    // 172.16.0.0/12
    if ((ip_addr & 0xFFF00000) == 0xAC100000) return true;
    
    // 192.168.0.0/16
    if ((ip_addr & 0xFFFF0000) == 0xC0A80000) return true;
    
    // 127.0.0.0/8 (localhost)
    if ((ip_addr & 0xFF000000) == 0x7F000000) return true;
    
    // 169.254.0.0/16 (link-local)
    if ((ip_addr & 0xFFFF0000) == 0xA9FE0000) return true;
    
    return false;
}

// Check if IPv6 address is in private range
bool is_private_ipv6(const uint32_t ip_addr[4])
{
    // ::1 (localhost)
    if (ip_addr[0] == 0 && ip_addr[1] == 0 && ip_addr[2] == 0 && ip_addr[3] == htonl(1)) {
        return true;
    }
    
    // fe80::/10 (link-local)
    if ((ip_addr[0] & htonl(0xFFC00000)) == htonl(0xFE800000)) {
        return true;
    }
    
    // fc00::/7 (unique local)
    if ((ip_addr[0] & htonl(0xFE000000)) == htonl(0xFC000000)) {
        return true;
    }
    
    return false;
}

// Get default interface name
int get_default_interface(char *interface, size_t interface_len)
{
    if (!interface || interface_len == 0) return -1;
    
    FILE *fp = fopen("/proc/net/route", "r");
    if (!fp) {
        log_error("Failed to open /proc/net/route: %s", strerror(errno));
        return -1;
    }
    
    char line[256];
    int found = 0;
    
    // Skip header
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        char iface_name[32];
        unsigned long destination;
        
        if (sscanf(line, "%31s %lx", iface_name, &destination) == 2) {
            if (destination == 0) { // Default gateway
                safe_string_copy(interface, iface_name, interface_len);
                found = 1;
                break;
            }
        }
    }
    
    fclose(fp);
    return found ? 0 : -1;
}

// Get interface IP address
int get_interface_ip(const char *interface, char *ip_str, size_t ip_str_len, bool ipv6)
{
    if (!interface || !ip_str) return -1;
    
    struct ifaddrs *ifaddrs_ptr, *ifa;
    
    if (getifaddrs(&ifaddrs_ptr) != 0) {
        log_error("getifaddrs failed: %s", strerror(errno));
        return -1;
    }
    
    int result = -1;
    
    for (ifa = ifaddrs_ptr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        if (strcmp(ifa->ifa_name, interface) != 0) continue;
        
        if (ipv6 && ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            if (inet_ntop(AF_INET6, &addr6->sin6_addr, ip_str, ip_str_len)) {
                result = 0;
                break;
            }
        } else if (!ipv6 && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &addr->sin_addr, ip_str, ip_str_len)) {
                result = 0;
                break;
            }
        }
    }
    
    freeifaddrs(ifaddrs_ptr);
    return result;
}

// Check if interface exists
bool interface_exists(const char *interface)
{
    if (!interface) return false;
    
    struct ifaddrs *ifaddrs_ptr, *ifa;
    
    if (getifaddrs(&ifaddrs_ptr) != 0) {
        return false;
    }
    
    bool exists = false;
    for (ifa = ifaddrs_ptr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_name && strcmp(ifa->ifa_name, interface) == 0) {
            exists = true;
            break;
        }
    }
    
    freeifaddrs(ifaddrs_ptr);
    return exists;
}

// Calculate IP header checksum
uint16_t calculate_ip_checksum(const uint8_t *header, size_t header_len)
{
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)header;
    
    while (header_len > 1) {
        sum += *ptr++;
        header_len -= 2;
    }
    
    if (header_len == 1) {
        sum += *(uint8_t *)ptr;
    }
    
    // Fold to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)~sum;
}

// Validate port number
bool is_valid_port(uint16_t port)
{
    return port > 0 && port <= 65535;
}

// Check if port is privileged (< 1024)
bool is_privileged_port(uint16_t port)
{
    return port > 0 && port < 1024;
}
