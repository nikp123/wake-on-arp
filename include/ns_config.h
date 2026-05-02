#include <stdbool.h>
#include <stddef.h>
#include <time.h>
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
 * ns_config.h
 *
 *  Created on			: 03-Nov-2015
 *  Author				: rp
 *  Date					: 12:52:52 am
 */

#ifndef NS_CONFIG_H_
#define NS_CONFIG_H_

#define DEFAULT_NETWORK_INTERFACE				"eth0"
#define DEFAULT_BUF_SIZE							 			2048

#define DEFAULT_ARP_RESPONSE_ITERATION		5

/* useful for coding */
#define PUBLIC
#define PRIVATE														static

#define IN																	const
#define OUT
#define INOUT

// DNS cache refresh interval in seconds
#define DEFAULT_DNS_CACHE_REFRESH_INTERVAL  300

// Maximum number of entries in allow/exclude lists
#define MAX_SOURCE_LIST_ENTRIES 64

// Structure for a source entry (IP or hostname)
typedef struct {
	char *entry; // IP string or hostname
	uint32_t ip; // resolved IP (if available)
	bool is_hostname;
} source_entry_t;

// DNS cache for hostnames in allow/exclude lists
typedef struct {
	source_entry_t entries[MAX_SOURCE_LIST_ENTRIES];
	size_t count;
	time_t last_refresh;
} dns_cache_t;

#endif /* NS_CONFIG_H_ */
