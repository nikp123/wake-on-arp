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
 *ns_ipv4.h
 *
 *  Created on			: 05-May-2016
 *  Author				: rp
 *  Date					: 3:24:27 pm
 */

#ifndef NS_IPV4_H_
#define NS_IPV4_H_

#include <stdint.h>						/* uint16_t and likes */

#include "ns_config.h"

#define NS_IPv4_ICMP_PROTO					1
#define NS_IPv4_IGMP_PROTO					2
#define NS_IPv4_TCP_PROTO						6
#define NS_IPv4_UDP_PROTO						17

#define NS_IPv4_TYPE_STR_LEN					8

/*
 * IP structure
 */
typedef struct _ns_IPv4_packet {
#if __BYTE_ORDER__ == __LITTLE_ENDIAN__
		uint8_t ns_ipv4_hdr_len :4;
		uint8_t ns_ipv4_ver :4;
#else
		uint8_t ns_ipv4_ver :4;
		uint8_t ns_ipv4_hdr_len :4;
#endif
		uint8_t ns_ipv4_tos;
		uint16_t ns_ipv4_len; /* data  & header */
		uint16_t ns_ipv4_id;
		uint16_t ns_ipv4_offset; /* flags + fragment offset */
		uint8_t ns_ipv4_ttl;
		uint8_t ns_ipv4_proto;
		uint16_t ns_ipv4_checksum;

		uint32_t ns_ipv4_src;
		uint32_t ns_ipv4_dest;
}__attribute__ ((packed)) ns_IPv4_packet_t;

/*
 * exported function declarations
 */
PUBLIC ns_error_t parse_ipv4_packet(IN unsigned char *);

#endif /* NS_IPV4_H_ */
