#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// Raw socket for packet capture and injection
static int raw_socket_fd = -1;
static int raw_socket_ipv6_fd = -1;

// Initialize raw socket
int setup_raw_socket(void)
{
    // Create IPv4 raw socket
    raw_socket_fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (raw_socket_fd < 0) {
        log_error("Failed to create IPv4 raw socket: %s", strerror(errno));
        return -1;
    }
    
    // Create IPv6 raw socket
    raw_socket_ipv6_fd = socket(AF_INET6, SOCK_RAW, IPPROTO_TCP);
    if (raw_socket_ipv6_fd < 0) {
        log_error("Failed to create IPv6 raw socket: %s", strerror(errno));
        close(raw_socket_fd);
        raw_socket_fd = -1;
        return -1;
    }
    
    // Set socket options for better performance
    int sock_buf_size = 1024 * 1024; // 1MB buffer
    if (setsockopt(raw_socket_fd, SOL_SOCKET, SO_RCVBUF, &sock_buf_size, sizeof(sock_buf_size)) < 0) {
        log_warning("Failed to set IPv4 socket buffer size: %s", strerror(errno));
    }
    
    if (setsockopt(raw_socket_ipv6_fd, SOL_SOCKET, SO_RCVBUF, &sock_buf_size, sizeof(sock_buf_size)) < 0) {
        log_warning("Failed to set IPv6 socket buffer size: %s", strerror(errno));
    }
    
    log_info("Raw sockets initialized successfully");
    return 0;
}

// Close raw sockets
void cleanup_raw_socket(void)
{
    if (raw_socket_fd >= 0) {
        close(raw_socket_fd);
        raw_socket_fd = -1;
    }
    
    if (raw_socket_ipv6_fd >= 0) {
        close(raw_socket_ipv6_fd);
        raw_socket_ipv6_fd = -1;
    }
    
    log_info("Raw sockets cleaned up");
}

// Send raw packet
int send_raw_packet(const uint8_t *packet_data, size_t packet_len, bool is_ipv6)
{
    int sock_fd = is_ipv6 ? raw_socket_ipv6_fd : raw_socket_fd;
    
    if (sock_fd < 0) {
        log_error("Raw socket not initialized for %s", is_ipv6 ? "IPv6" : "IPv4");
        return -1;
    }
    
    ssize_t sent = send(sock_fd, packet_data, packet_len, 0);
    if (sent < 0) {
        log_error("Failed to send raw packet: %s", strerror(errno));
        return -1;
    }
    
    if ((size_t)sent != packet_len) {
        log_warning("Packet sent partially: %zd/%zu bytes", sent, packet_len);
    }
    
    log_debug("Sent raw packet: %zu bytes via %s", packet_len, is_ipv6 ? "IPv6" : "IPv4");
    return 0;
}

// Receive raw packet (non-blocking)
int receive_raw_packet(uint8_t *buffer, size_t buffer_len, bool is_ipv6, 
                      struct sockaddr *src_addr, socklen_t *addr_len)
{
    int sock_fd = is_ipv6 ? raw_socket_ipv6_fd : raw_socket_fd;
    
    if (sock_fd < 0) {
        log_error("Raw socket not initialized for %s", is_ipv6 ? "IPv6" : "IPv4");
        return -1;
    }
    
    ssize_t received = recvfrom(sock_fd, buffer, buffer_len, MSG_DONTWAIT, 
                              src_addr, addr_len);
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No data available
        }
        log_error("Failed to receive raw packet: %s", strerror(errno));
        return -1;
    }
    
    log_debug("Received raw packet: %zd bytes via %s", received, is_ipv6 ? "IPv6" : "IPv4");
    return (int)received;
}

// Check if raw sockets are ready
bool is_raw_socket_ready(void)
{
    return (raw_socket_fd >= 0 && raw_socket_ipv6_fd >= 0);
}

// Get socket file descriptors for select/poll
int get_raw_socket_fds(int *ipv4_fd, int *ipv6_fd)
{
    if (ipv4_fd) *ipv4_fd = raw_socket_fd;
    if (ipv6_fd) *ipv6_fd = raw_socket_ipv6_fd;
    
    return (raw_socket_fd >= 0 && raw_socket_ipv6_fd >= 0) ? 0 : -1;
}