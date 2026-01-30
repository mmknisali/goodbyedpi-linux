#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include "../include/config.h"
#include "../include/packet.h"

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
        
        if (config.host_mixedcase) {
            // TODO: Implement host header case mixing
            modified = 1;
        }
        
        if (config.host_removespace) {
            // TODO: Implement space removal after Host:
            modified = 1;
        }
        
        if (config.additional_space) {
            // TODO: Implement additional space
            modified = 1;
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