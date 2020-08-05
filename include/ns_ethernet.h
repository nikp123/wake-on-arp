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
 * ns_ethernet.h
 *
 *  Created on			: 03-Nov-2015
 *  Author				: rp
 *  Date					: 11:00:19 pm
 */

#ifndef NS_ETHERNET_H_
#define NS_ETHERNET_H_

#include <stdint.h>			/* uint16_t and likes */
#include <stddef.h>		/* NULL */

#include "ns_error.h"		/* error states */
#include "ns_config.h"

/************************************************************
 * ETHERNET FRAME FORMAT
 *
 * 		6			6			2	    (var)		4		Bytes/Octets
 * 	-----------------------------------------------------------------
 * 	| destination	| source 		| ether	| payload	| CRC	|
 * 	| mac addr	| mac addr	| type	|		|		|
 * 	-----------------------------------------------------------------
 ************************************************************/

/*
 * Magic Constants as per the RFC
 */
#define NS_ETH_ADDR_LEN						6				/* # of octets in MAC addr 	*/
#define NS_ETH_TYPE_LEN						2				/* # of octets for type 		*/
#define NS_ETH_MIN_PAYLOAD_LEN		28			/* min octects in a payload 	*/
#define NS_ETH_MAX_PAYLOAD_LEN		1504		/* max octets in a payload 	*/

/*
 * Protocol ID
 */
#define NS_ETH_TYPE_IPv4						0x0800		/* IPv4 */
#define NS_ETH_TYPE_ARP						0x0806		/* Address Resolution packet	*/
#define NS_ETH_TYPE_IPv6						0x86DD		/* IPv6 */

/*
 * Ethernet-II (802.3) header
 */
#define NS_ETH_HDR_LEN																							\
	(																																	\
		NS_ETH_ADDR_LEN +	/* Dest MAC */																\
		NS_ETH_ADDR_LEN +	/* Src MAC */																	\
		NS_ETH_TYPE_LEN		/* Ether Type */																\
	)

typedef struct _ns_ethernet_frame_hdr {
		unsigned char ns_eth_dest[NS_ETH_ADDR_LEN];
		unsigned char ns_eth_src[NS_ETH_ADDR_LEN];
		uint16_t ns_eth_type;
} ns_ethernet_frame_hdr_t;

/*
 * minimum # of octets in a Ethernet-II (802.3) frame (sans CRC)
 */
#define NS_ETH_MIN_LEN																							\
	(																																	\
		NS_ETH_HDR_LEN + 				/* ethernet hdr */												\
		NS_ETH_MIN_PAYLOAD_LEN	/* min payload */												\
	)

/*
 * maximum # of octets in a Ethernet-II (802.3) frame (sans CRC)
 */
#define NS_ETH_MAX_LEN																							\
	(																																	\
		NS_ETH_HDR_LEN + 				/* ethernet hdr */												\
		NS_ETH_MAX_PAYLOAD_LEN	/* max payload */												\
	)

/*
 * print len
 */
#define NS_ETH_TYPE_STR_LEN					6

PUBLIC ns_error_t parse_ethernet_packet(IN unsigned char *, IN int,
	OUT unsigned char**, OUT uint16_t*);

#endif /* NS_ETHERNET_H_ */
