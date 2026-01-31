#ifndef PACKET_H
#define PACKET_H

#include "goodbyedpi.h"

// Packet parsing constants
#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif

#ifndef IPPROTO_UDP
#define IPPROTO_UDP 17
#endif

// TCP flags
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20
#define TCP_ECE 0x40
#define TCP_CWR 0x80

// IP header sizes
#define IP_HDR_LEN sizeof(struct iphdr)
#define IPV6_HDR_LEN sizeof(struct ipv6hdr)
#define TCP_HDR_LEN sizeof(struct tcphdr)
#define UDP_HDR_LEN sizeof(struct udphdr)

#ifndef GOODBYEDPI_PACKET_MACROS
#define GOODBYEDPI_PACKET_MACROS


#ifndef GOODBYEDPI_IP_TOS
#define GOODBYEDPI_IP_TOS(ip_hdr)  ((ip_hdr)->tos)
#endif

#ifndef GOODBYEDPI_IP_TTL
#define GOODBYEDPI_IP_TTL(ip_hdr)   ((ip_hdr)->ttl)
#endif

// IPv4 structure access macros
#define IP_V(ip_hdr)    ((ip_hdr)->version)
#define IP_HL(ip_hdr)   ((ip_hdr)->ihl)
#define IP_TOS(ip_hdr)  ((ip_hdr)->tos)
#define IP_LEN(ip_hdr)   ((ip_hdr)->tot_len)
#define IP_ID(ip_hdr)    ((ip_hdr)->id)
#define IP_OFF(ip_hdr)   ((ip_hdr)->frag_off)
#define IP_TTL(ip_hdr)   ((ip_hdr)->ttl)
#define IP_P(ip_hdr)     ((ip_hdr)->protocol)
#define IP_SUM(ip_hdr)   ((ip_hdr)->check)
#define IP_SRC(ip_hdr)   ((ip_hdr)->saddr)
#define IP_DST(ip_hdr)   ((ip_hdr)->daddr)

// TCP structure access macros
#define TH_OFF(th)      ((th)->doff)
#define TH_FLAGS(th)     ((th)->flags)
#define TCP_SPORT(th)    ((th)->source)
#define TCP_DPORT(th)    ((th)->dest)
#define TCP_SEQ(th)      ((th)->seq)
#define TCP_ACK(th)      ((th)->ack_seq)
#define TCP_WIN(th)      ((th)->window)
#define TCP_SUM(th)      ((th)->check)
#define TCP_URP(th)      ((th)->urg_ptr)

// IPv6 structure access macros
#define IPV6_V(ipv6_hdr)    ((ipv6_hdr)->version)
#define IPV6_TC(ipv6_hdr)   ((ipv6_hdr)->priority)
#define IPV6_FL(ipv6_hdr)   ((ipv6_hdr)->flow_lbl)
#define IPV6_NXT(ipv6_hdr)  ((ipv6_hdr)->nexthdr)
#define IPV6_LIM(ipv6_hdr)  ((ipv6_hdr)->payload_len)

// UDP structure access macros
#define UDP_SPORT(uh)    ((uh)->source)
#define UDP_DPORT(uh)    ((uh)->dest)
#define UDP_LEN(uh)      ((uh)->len)
#define UDP_SUM(uh)      ((uh)->check)

// Packet parsing functions
int packet_init(packet_t *packet);
void packet_free(packet_t *packet);
int packet_parse_ipv4(const uint8_t *data, size_t len, packet_t *packet);
int packet_parse_ipv6(const uint8_t *data, size_t len, packet_t *packet);

int packet_is_udp(const packet_t *packet);
int packet_is_http(const packet_t *packet);
int packet_is_https(const packet_t *packet);

// Packet modification functions
int packet_copy(const packet_t *src, packet_t *dst);
int packet_set_ttl(packet_t *packet, uint8_t ttl);
int packet_modify_tcp_checksum(packet_t *packet);
int packet_modify_ip_checksum(packet_t *packet);
int packet_calculate_checksums(packet_t *packet);

// Fragmentation functions
int packet_split(packet_t *packet, packet_t *first, packet_t *second, size_t split_point);
int packet_create_fragment(const packet_t *original, packet_t *fragment, 
                        size_t offset, size_t fragment_size, int is_first);

// Header manipulation functions
int packet_get_http_method(const packet_t *packet, char *method, size_t method_len);
int packet_get_http_host(const packet_t *packet, char *host, size_t host_len);
int packet_get_http_user_agent(const packet_t *packet, char *user_agent, size_t ua_len);
int packet_set_http_host(packet_t *packet, const char *new_host);
int packet_mix_host_case(packet_t *packet);
int packet_remove_host_space(packet_t *packet);
int packet_add_additional_space(packet_t *packet);

// Utility functions
uint32_t packet_checksum(const uint16_t *data, size_t len);
uint16_t ip_checksum(const void *data, size_t len);
uint16_t tcp_checksum(const void *data, size_t len, uint32_t src_ip, uint32_t dst_ip);
void print_packet_info(const packet_t *packet);

#endif // PACKET_H
