#ifndef NETFILTER_CAPTURE_H
#define NETFILTER_CAPTURE_H

#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libmnl/libmnl.h>
#include "goodbyedpi.h"
#include "packet.h"

// Netfilter queue context
typedef struct {
    struct nfq_handle *nfq_handle;
    struct nfq_q_handle *queue_handle;
    int fd;
    uint16_t queue_num;
    bool initialized;
    char buffer[MAX_PACKET_SIZE * 2];  // Buffer for receiving packets
    size_t buffer_len;
} netfilter_context_t;

// Netfilter verdicts
#define NF_DROP 0
#define NF_ACCEPT 1
#define NF_STOLEN 2
#define NF_QUEUE 3
#define NF_REPEAT 4
#define NF_STOP 5

// Netfilter queue callback
typedef int (*packet_callback_t)(struct nfq_q_handle *qh, 
                               struct nfgenmsg *nfmsg,
                               struct nfq_data *nfa,
                               void *data);

// Core netfilter functions
int netfilter_init(netfilter_context_t *ctx, uint16_t queue_num, packet_callback_t callback);
int netfilter_cleanup(netfilter_context_t *ctx);
int netfilter_receive_packet(netfilter_context_t *ctx);
int netfilter_send_verdict(netfilter_context_t *ctx, uint32_t packet_id, 
                          int verdict, const uint8_t *data, size_t data_len);

// Packet handling via netfilter
int netfilter_get_packet_data(struct nfq_data *nfa, uint8_t **packet_data, uint32_t *packet_len);
int netfilter_get_packet_metadata(struct nfq_data *nfa, uint32_t *packet_id, 
                                 uint32_t *in_dev, uint32_t *out_dev,
                                 struct nfqnl_msg_packet_hw *hw_addr, uint16_t *hw_addrlen);
int netfilter_parse_packet(const uint8_t *data, uint32_t len, packet_t *packet);

// Queue management
int netfilter_set_queue_maxlen(netfilter_context_t *ctx, uint32_t maxlen);
int netfilter_set_mode(netfilter_context_t *ctx, uint8_t mode, uint32_t range);

// Error handling
const char *netfilter_error_string(int err);
void netfilter_print_error(const char *operation, int err);

// Utility functions
uint32_t netfilter_get_packet_timestamp(struct nfq_data *nfa);
int netfilter_get_packet_mark(struct nfq_data *nfa, uint32_t *mark);
int netfilter_set_packet_mark(uint32_t packet_id, uint32_t mark);

#endif // NETFILTER_CAPTURE_H