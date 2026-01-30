#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include "../include/netfilter_capture.h"
#include <linux/netfilter.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>

// Global callback for packet processing
static packet_callback_t g_packet_callback = NULL;

// Netfilter queue callback function
static int netfilter_callback(struct nfq_q_handle *qh, 
                            struct nfgenmsg *nfmsg,
                            struct nfq_data *nfa,
                            void *data)
{
    uint32_t packet_id = 0;
    uint8_t *packet_data = NULL;
    uint32_t packet_len = 0;
    int verdict = NF_ACCEPT;
    
    // Get packet ID
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
    if (ph) {
        packet_id = ntohl(ph->packet_id);
    }
    
    // Get packet data
    packet_len = nfq_get_payload(nfa, (unsigned char **)&packet_data);
    if (packet_len == 0) {
        printf("Empty packet received\n");
        goto send_verdict;
    }
    
    printf("Received packet: id=%u, len=%u\n", packet_id, packet_len);
    
    // Call the user callback if set
    if (g_packet_callback) {
        verdict = g_packet_callback(qh, nfmsg, nfa, data);
    } else {
        // Default behavior: accept all packets
        verdict = NF_ACCEPT;
    }
    
send_verdict:
    return nfq_set_verdict(qh, packet_id, verdict, 0, NULL);
}

// Initialize netfilter queue
int netfilter_init(netfilter_context_t *ctx, uint16_t queue_num, packet_callback_t callback)
{
    if (!ctx) {
        printf("Invalid context\n");
        return -1;
    }
    
    memset(ctx, 0, sizeof(netfilter_context_t));
    ctx->queue_num = queue_num;
    
    // Open netfilter handle
    ctx->nfq_handle = nfq_open();
    if (!ctx->nfq_handle) {
        printf("nfq_open failed: %s\n", strerror(errno));
        return -1;
    }
    
    // Unbind existing queue handler (if any)
    if (nfq_unbind_pf(ctx->nfq_handle, AF_INET) < 0) {
        printf("nfq_unbind_pf failed: %s\n", strerror(errno));
        nfq_close(ctx->nfq_handle);
        return -1;
    }
    
    if (nfq_unbind_pf(ctx->nfq_handle, AF_INET6) < 0) {
        printf("nfq_unbind_pf (IPv6) failed: %s\n", strerror(errno));
        nfq_close(ctx->nfq_handle);
        return -1;
    }
    
    // Bind to protocol families
    if (nfq_bind_pf(ctx->nfq_handle, AF_INET) < 0) {
        printf("nfq_bind_pf failed: %s\n", strerror(errno));
        nfq_close(ctx->nfq_handle);
        return -1;
    }
    
    if (nfq_bind_pf(ctx->nfq_handle, AF_INET6) < 0) {
        printf("nfq_bind_pf (IPv6) failed: %s\n", strerror(errno));
        nfq_close(ctx->nfq_handle);
        return -1;
    }
    
    // Create queue
    ctx->queue_handle = nfq_create_queue(ctx->nfq_handle, queue_num, 
                                     &netfilter_callback, ctx);
    if (!ctx->queue_handle) {
        printf("nfq_create_queue failed: %s\n", strerror(errno));
        nfq_close(ctx->nfq_handle);
        return -1;
    }
    
    // Set queue mode to copy entire packet
    if (nfq_set_mode(ctx->queue_handle, NFQNL_COPY_PACKET, 0xffff) < 0) {
        printf("nfq_set_mode failed: %s\n", strerror(errno));
        nfq_destroy_queue(ctx->queue_handle);
        nfq_close(ctx->nfq_handle);
        return -1;
    }
    
    // Get file descriptor
    ctx->fd = nfq_fd(ctx->nfq_handle);
    if (ctx->fd < 0) {
        printf("nfq_fd failed\n");
        nfq_destroy_queue(ctx->queue_handle);
        nfq_close(ctx->nfq_handle);
        return -1;
    }
    
    // Store callback
    g_packet_callback = callback;
    
    ctx->initialized = true;
    printf("Netfilter queue initialized: queue_num=%u, fd=%d\n", queue_num, ctx->fd);
    
    return 0;
}

// Cleanup netfilter queue
int netfilter_cleanup(netfilter_context_t *ctx)
{
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    if (ctx->queue_handle) {
        nfq_destroy_queue(ctx->queue_handle);
    }
    
    if (ctx->nfq_handle) {
        nfq_close(ctx->nfq_handle);
    }
    
    ctx->initialized = false;
    printf("Netfilter queue cleaned up\n");
    
    return 0;
}

// Receive packet from netfilter queue
int netfilter_receive_packet(netfilter_context_t *ctx)
{
    int len;
    char buf[4096];
    
    if (!ctx || !ctx->initialized) {
        printf("Netfilter not initialized\n");
        return -1;
    }
    
    len = recv(ctx->fd, buf, sizeof(buf), 0);
    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;  // No data available
        }
        printf("recv failed: %s\n", strerror(errno));
        return -1;
    }
    
    ctx->buffer_len = len;
    
    // Process the message
    if (nfq_handle_packet(ctx->nfq_handle, buf, len) < 0) {
        printf("nfq_handle_packet failed: %s\n", strerror(errno));
        return -1;
    }
    
    printf("Received %d bytes from netfilter\n", len);
    return len;
}

// Send verdict for packet
int netfilter_send_verdict(netfilter_context_t *ctx, uint32_t packet_id, 
                          int verdict, const uint8_t *data, size_t data_len)
{
    if (!ctx || !ctx->initialized) {
        return -1;
    }
    
    int result = nfq_set_verdict(ctx->queue_handle, packet_id, verdict, data_len, data);
    if (result < 0) {
        printf("nfq_set_verdict failed: %s\n", strerror(errno));
        return -1;
    }
    
    printf("Sent verdict: id=%u, verdict=%d, data_len=%zu\n", packet_id, verdict, data_len);
    return 0;
}

// Get packet data from netfilter structure
int netfilter_get_packet_data(struct nfq_data *nfa, uint8_t **packet_data, uint32_t *packet_len)
{
    if (!nfa || !packet_data || !packet_len) {
        return -1;
    }
    
    *packet_len = nfq_get_payload(nfa, (unsigned char **)packet_data);
    return (*packet_len > 0) ? 0 : -1;
}

// Get packet metadata
int netfilter_get_packet_metadata(struct nfq_data *nfa, uint32_t *packet_id, 
                                 uint32_t *in_dev, uint32_t *out_dev,
                                 struct nfqnl_msg_packet_hw *hw_addr, uint16_t *hw_addrlen)
{
    if (!nfa || !packet_id) {
        return -1;
    }
    
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
    if (ph) {
        *packet_id = ntohl(ph->packet_id);
        
        if (in_dev) *in_dev = nfq_get_indev(nfa);
        if (out_dev) *out_dev = nfq_get_outdev(nfa);
        if (hw_addr) *hw_addr = nfq_get_packet_hw(nfa, hw_addrlen);
        if (hw_addrlen) *hw_addrlen = nfq_get_packet_hw(nfa, hw_addrlen);
    }
    
    return 0;
}

// Print error
void netfilter_print_error(const char *operation, int err)
{
    printf("Netfilter %s failed: %s\n", operation, strerror(err));
}