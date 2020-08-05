/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Rp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * ns_arp.h
 *
 *  Created on			: 03-Nov-2015
 *  Author				: rp
 *  Date					: 8:46:46 pm
 */

#ifndef NS_ARP_H_
#define NS_ARP_H_

#include "ns_config.h"

#include "ns_utils.h"
#include "ns_ethernet.h"
#include "ns_error.h"

#include "ns_arp_packet.h"

/*
 * IPv4 over Ethernet ARP payload
 */
typedef struct _ns_arp_IPv4_eth_packet {
		ns_arp_packet_hdr_t ns_arp_hdr;

		/* addresses */
		unsigned char ns_arp_sender_hw_addr[NS_ETH_ADDR_LEN];
		unsigned char ns_arp_sender_proto_addr[NS_IPv4_ADDR_LEN];
		unsigned char ns_arp_target_hw_addr[NS_ETH_ADDR_LEN];
		unsigned char ns_arp_target_proto_addr[NS_IPv4_ADDR_LEN];
} ns_arp_IPv4_eth_packet_t;

/*
 * exported function declarations
 */
PUBLIC ns_error_t parse_arp_packet(IN unsigned char *);

#endif /* NS_ARP_H_ */
