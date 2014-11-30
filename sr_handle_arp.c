#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sr_protocol.h"
#include "sr_router.h"
#include "sr_utils.h"
#include "sr_handle_arp.h"

void sr_handle_arp(struct sr_instance* sr,
    uint8_t *packet, unsigned int len, char *interface) {
  // Get packet arp header
  sr_arp_hdr_t *arp_hdr = packet_get_arp_hdr(packet);

  if(ntohs(arp_hdr->ar_op) == arp_op_request)
    sr_handle_arp_req(sr, packet, len, interface);
  else if(ntohs(arp_hdr->ar_op) == arp_op_reply)
    sr_handle_arp_rep(sr, packet, len, interface);
}

/**
   # When sending packet to next_hop_ip
   entry = arpcache_lookup(next_hop_ip)

   if entry:
       use next_hop_ip->mac mapping in entry to send the packet
       free entry
   else:
       req = arpcache_queuereq(next_hop_ip, packet, len)
       handle_arpreq(req)
       free(packet) // not sure if I sould do this
**/
void sr_handle_arp_req(struct sr_instance* sr,
    uint8_t *packet, unsigned int len, char* interface) {
  sr_ethernet_hdr_t *req_eth_hdr = packet_get_eth_hdr(packet);
  sr_arp_hdr_t *req_arp_hdr = packet_get_arp_hdr(packet);
  struct sr_arpentry *entry = sr_arpcache_lookup(&sr->cache, req_arp_hdr->ar_tip);

  // Get interface of router this arp req was received on
  struct sr_if *iface = sr_get_interface(sr, interface);

  // If the ARP req was for this me, respond with ARP reply
  if(req_arp_hdr->ar_tip == iface->ip) {
    Debug("Request for this router, constructing reply");

    unsigned int new_len = sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t);
    uint8_t *rep_packet = (uint8_t *)malloc(new_len);
    bzero(rep_packet, new_len);


    // Construct ARP ethernet hdr
    construct_arp_eth_hdr_at(rep_packet, req_eth_hdr, iface);

    // Construct ARP hdr
    construct_arp_hdr_at(rep_packet + sizeof(sr_ethernet_hdr_t),
        req_arp_hdr, iface);

    //print_hdrs(rep_packet, new_len);

    // Put our new packet back on the wire
    sr_send_packet(sr, (uint8_t *)rep_packet, new_len, interface);
    free(rep_packet);
  }

  // We don't know about this host already, so add it
  if(entry != NULL) {
  }
  // Otherwise add it to ARP cache, refreshing timeout memory
  else {
    //sr_print_if(iface);
  }

}

void construct_arp_eth_hdr_at(uint8_t *buf, sr_ethernet_hdr_t *eth_hdr, struct sr_if *iface) {
  // Construct ethernet hdr
  struct sr_ethernet_hdr *rep_eth_hdr = (sr_ethernet_hdr_t *)buf;
  // set destination to origin
  memcpy(rep_eth_hdr->ether_dhost,
      eth_hdr->ether_shost, ETHER_ADDR_LEN);
  //set source to our interface's eth addr
  memcpy(rep_eth_hdr->ether_shost,
      iface->addr, ETHER_ADDR_LEN);
  // ethernet type is ARP
  rep_eth_hdr->ether_type = ntohs(ethertype_arp);
}

void construct_arp_hdr_at(uint8_t *buf, sr_arp_hdr_t *arp_hdr,
    struct sr_if *iface) {
    sr_arp_hdr_t *rep_arp_hdr = (sr_arp_hdr_t *)buf;
    rep_arp_hdr->ar_hrd = arp_hdr->ar_hrd; // 1 for ethernet
    rep_arp_hdr->ar_pro = arp_hdr->ar_pro; // protocol format is IPv4 (0x800)
    rep_arp_hdr->ar_hln = arp_hdr->ar_hln; // hardware length is same (6)
    rep_arp_hdr->ar_pln = arp_hdr->ar_pln; // protocol length is same (4)
    rep_arp_hdr->ar_op = htons(arp_op_reply); // ARP reply
    memcpy(rep_arp_hdr->ar_sha,
        iface->addr, ETHER_ADDR_LEN); // set hw addr
    rep_arp_hdr->ar_sip = iface->ip; // setting us as sender
    memcpy(rep_arp_hdr->ar_tha,
        arp_hdr->ar_sha, ETHER_ADDR_LEN); // target
    rep_arp_hdr->ar_tip = arp_hdr->ar_sip;
}

/**
   The ARP reply processing code should move entries from the ARP request
   queue to the ARP cache:

   # When servicing an arp reply that gives us an IP->MAC mapping
   req = arpcache_insert(ip, mac)

   if req:
       send all packets on the req->packets linked list
       arpreq_destroy(req)
 **/
void sr_handle_arp_rep(struct sr_instance* sr, uint8_t *packet, unsigned int len, char* interface) {
  sr_arp_hdr_t *arp_hdr = packet_get_arp_hdr(packet);
  printf("lol2\n");
}
