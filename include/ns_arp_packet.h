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
 * ns_arp_packet.h
 *
 *  Created on		: 03-Nov-2015
 *  Author				: rp
 *  Date					: 10:50:46 pm
 */

#ifndef NS_ARP_PACKET_H_
#define NS_ARP_PACKET_H_

#include <stdint.h>						/* uint16_t and likes */

/************************************************************
 * ARP FRAME HDR FORMAT
 *
 *      2      2         1          1          2         Bytes/Octets
 *  -----------------------------------------------
 *  | HW   | Proto | HW Addr  | Proto Addr | Op   |
 *  | Type | Type  |   Len    |    Len     | Code |
 *  -----------------------------------------------
 ************************************************************/

/*
 * Magic Constants as per the RFC
 */
#define NS_ARP_ETHERNET_TYPE					1				/* Ethernet 10/100Mbps */

/*
 * ARP protocol opcodes
 */
#define NS_ARP_REQUEST								1				/* ARP request. */
#define NS_ARP_REPLY									2 				/* ARP reply. */

typedef struct _ns_arp_packet_hdr {
	uint16_t ns_arp_hw_type;
	uint16_t ns_arp_proto_type;
	uint8_t ns_arp_hw_addr_len;
	uint8_t ns_arp_proto_addr_len;
	uint16_t ns_arp_opcode;
} ns_arp_packet_hdr_t;

/* ToDo: Move to IPv4 based file */
#define NS_IPv4_ADDR_LEN						4			/* Length of IPv4 addr */
#define NS_ARP_TYPE_STR_LEN					8

#define NS_ARP_REPLY_BUF_LEN				42		/* Size of ARP reply */

#endif /* NS_ARP_PACKET_H_ */
