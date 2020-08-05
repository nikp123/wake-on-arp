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
 * ns_utils.h
 *
 *  Created on			: 03-Nov-2015
 *  Author				: rp
 *  Date					: 7:48:19 pm
 */

#ifndef NS_UTILS_H_
#define NS_UTILS_H_

#include <stdio.h>
#include <arpa/inet.h>

#include "ns_error.h"
#include "ns_config.h"

#define MAC_ADDR_LEN											6
#define IPv4_ADDR_LEN											4

/* printable MAC address length */
#define NS_ETH_IPv4_PRINTABLE_MAC_SIZE		24
#define NS_ETH_IPv4_PRINTABLE_IPv4_SIZE		24

PUBLIC void human_readable_MAC(IN unsigned char*, OUT char*);
PUBLIC void human_readable_IPv4(IN unsigned char*, OUT char*);

PUBLIC char* human_readable_IPV4_from_number(IN uint32_t ip, char* ip_str);

PUBLIC ns_error_t get_ip_addr_from_name(IN char*, OUT struct in_addr*);
PUBLIC ns_error_t get_MAC_from_device_name(IN char*, OUT unsigned char*);

#endif /* NS_UTILS_H_ */
