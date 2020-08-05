/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Rp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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
 * ns_error.h
 *
 *  Created on			: 03-Nov-2015
 *  Author				: rp
 *  Date					: 1:00:53 am
 */

#ifndef NS_ERROR_H_
#define NS_ERROR_H_

typedef enum {
	/* generic system failures */
	ns_success,
	ns_failure,
	ns_malloc_failed,

	/* generic network failures */
	ns_socket_failed,
	ns_recvfrom_received_no_data,
	ns_unknown_host,
	ns_ioctl_failed,
	ns_interface_error,
	ns_sendto_failed,
	ns_bind_failed,

	/* ethernet */
	ns_eth_empty_packet,
	ns_eth_bad_packet_size,
	ns_eth_broadcast,

	/* arp */
	ns_not_ethernet_arp_packet,
	ns_not_ipv4_arp_packet,
	ns_not_ipv4_arp_request_packet
} ns_error_t;

#endif /* NS_ERROR_H_ */
