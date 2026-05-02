#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <byteswap.h>

#include "ns_config.h"
#include "ns_arp.h"
#include "ns_arp_packet.h"
#include "array.h"
#include "functions.h"

#define DEBUG_PRINT(...) do { if (m.debug) printf(__VA_ARGS__); } while(0)

// Main program state
struct main {
    char *eth_dev_s;
    char *eth_ip_s;
    char *broadcast_ip_s;
    char *subnet_s;
    char *allow_gateway_s;

    struct target *target_list;

    uint32_t *source_blacklist;
    uint32_t *source_allowlist;

    // DNS cache for hostnames in allow/exclude lists
    dns_cache_t exclude_dns_cache;
    dns_cache_t include_dns_cache;
    int dns_cache_refresh_interval;

    unsigned char eth_ip[4];
    unsigned char gate_ip[4];

    unsigned int  subnet;

    unsigned char *buffer;
    int sock_raw;
    bool alive;
    bool debug;
} m;
// Utility: resolve hostname to IPv4 address (returns 0 on success)
int resolve_hostname(const char *hostname, uint32_t *out_ip) {
	if (!hostname || !out_ip) return -1;
	
	struct addrinfo hints, *res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	int err = getaddrinfo(hostname, NULL, &hints, &res);
	if (err != 0 || !res) return -1;
	
	struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
	if (!addr || res->ai_addrlen < sizeof(struct sockaddr_in)) {
		freeaddrinfo(res);
		return -1;
	}
	
	*out_ip = addr->sin_addr.s_addr;
	freeaddrinfo(res);
	return 0;
}

// Non-blocking DNS cache refresh (only refresh if really needed and been a while)
void refresh_dns_cache_if_needed(dns_cache_t *cache) {
	if (!cache) return;
	
	time_t now = time(NULL);
	if (cache->last_refresh == 0 || (now - cache->last_refresh) >= DEFAULT_DNS_CACHE_REFRESH_INTERVAL) {
		DEBUG_PRINT("Refreshing DNS cache with %zu entries\n", cache->count);
		
		// Bounds check to prevent buffer overflow
		size_t safe_count = (cache->count > MAX_SOURCE_LIST_ENTRIES) ? MAX_SOURCE_LIST_ENTRIES : cache->count;
		
		for (size_t i = 0; i < safe_count; ++i) {
			// Additional safety checks
			if (!cache->entries[i].entry || strlen(cache->entries[i].entry) == 0) continue;
			
			if (cache->entries[i].is_hostname) {
				DEBUG_PRINT("Resolving hostname: %s\n", cache->entries[i].entry);
				uint32_t resolved_ip;
				if (resolve_hostname(cache->entries[i].entry, &resolved_ip) != 0) {
					cache->entries[i].ip = 0; // Mark as unresolved
					DEBUG_PRINT("Failed to resolve: %s\n", cache->entries[i].entry);
				} else {
					cache->entries[i].ip = resolved_ip;
				}
			} else {
				// Parse as IPv4 string
				uint8_t address[4];
				int err = sscanf(cache->entries[i].entry, "%hhu.%hhu.%hhu.%hhu",
					&address[0], &address[1], &address[2], &address[3]);
				if (err == 4) {
					// Safe conversion avoiding unaligned access
					cache->entries[i].ip = ((uint32_t)address[0]) |
					                       ((uint32_t)address[1] << 8) |
					                       ((uint32_t)address[2] << 16) |
					                       ((uint32_t)address[3] << 24);
				} else {
					cache->entries[i].ip = 0;
				}
			}
		}
		cache->last_refresh = now;
	}
}

// Non-blocking DNS cache refresh (only refresh if really needed and been a while)
void refresh_dns_cache_if_needed_nonblocking(dns_cache_t *cache) {
	if (!cache) return;
	
	static time_t last_any_refresh = 0;
	time_t now = time(NULL);
	
	// Only refresh if it's been at least the full interval since ANY refresh
	if (last_any_refresh != 0 && (now - last_any_refresh) < DEFAULT_DNS_CACHE_REFRESH_INTERVAL) {
		return; // Not time yet, skip to avoid blocking
	}
	
	// Do the refresh (this might take time but only happens every 5 minutes)
	refresh_dns_cache_if_needed(cache);
	last_any_refresh = now;
}

#ifndef CONFIG_PREFIX
	#error "Please specify CONFIG_PREFIX (usually /etc on Linux)"
#endif

// RETurn ON FAILure
#define RETONFAIL(x) { int a = x; if(a) return a; }

// FAILure ON ARGumentS
#define FAILONARGS(i, max) { if(max==i+1) { \
				fprintf(stderr, "Invalid number of arguments!\n"); \
				return -1; } }

const char *USAGE_INFO = \
"This program is a daemon wakes up a device on the local\n"
"network based upon if the local system tries to access it\n"
"via LAN network.\n"
"These parameters can also be set in the config file,\n"
"which is located at "CONFIG_PREFIX"/wake-on-arp.conf\n"
"Usage:\n"
"\t-h/--help - this screen\n"
"\t-i - IP address of device to wake up\n"
"\t-m - MAC (hardware) address of device to wake up\n"
"\t-d - network device to check traffic from (eg. eth0)\n"
"\t-b - broadcast IP address (eg. 192.168.1.255)\n"
"\t-s - subnet IP mask (eg. 24)\n"
"\t-ag - send magic packet even if the ARP came from the router/gateway (disabled by default). "
"For further info look here: https://github.com/nikp123/wake-on-arp/issues/1#issuecomment-882708765\n"
"\t--debug - enable verbose debug output\n";

void cleanup();
void sig_handler();
int initialize();
int watch_packets();
int process_packet(unsigned char*);
int parse_arp(unsigned char *);
int parse_ethhdr(unsigned char*);
int get_local_ip();
int send_magic_packet(unsigned char*);

void cleanup() {
	DEBUG_PRINT("Cleaning up resources\n");
	
	if (m.source_blacklist) {
		arr_free(m.source_blacklist);
		m.source_blacklist = NULL;
	}
	if (m.source_allowlist) {
		arr_free(m.source_allowlist);
		m.source_allowlist = NULL;
	}
	if (m.target_list) {
		targets_destroy(m.target_list);
		m.target_list = NULL;
	}
	if (m.sock_raw > 0) {
		close(m.sock_raw);
		m.sock_raw = 0;
	}
	if (m.buffer) {
		free(m.buffer);
		m.buffer = NULL;
	}
	
	// Clean up DNS cache entries
	for (size_t i = 0; i < m.exclude_dns_cache.count && i < MAX_SOURCE_LIST_ENTRIES; ++i) {
		if (m.exclude_dns_cache.entries[i].entry) {
			free(m.exclude_dns_cache.entries[i].entry);
			m.exclude_dns_cache.entries[i].entry = NULL;
		}
	}
	for (size_t i = 0; i < m.include_dns_cache.count && i < MAX_SOURCE_LIST_ENTRIES; ++i) {
		if (m.include_dns_cache.entries[i].entry) {
			free(m.include_dns_cache.entries[i].entry);
			m.include_dns_cache.entries[i].entry = NULL;
		}
	}
}

// handle signals, such as CTRL-C
void sig_handler() {
	m.alive = false;
}

int initialize() {
	RETONFAIL(get_local_ip());

	// get gateway ipv4 :)
	RETONFAIL(get_gateway_ip((unsigned char*)&m.gate_ip, m.eth_dev_s));

	// attach signal handler
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = &sig_handler;
	sigaction(SIGINT, &action, NULL);  // close by CTRL-C
	sigaction(SIGTERM, &action, NULL); // close by task manager and/or kill

	// set alive flag
	m.alive = true;

	// allocate memory for storing packets
	m.buffer = (unsigned char *) malloc(65536);

	// open the socket
	m.sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)) ;

	// listen on a specific network device
	setsockopt(m.sock_raw, SOL_SOCKET, SO_BINDTODEVICE, m.eth_dev_s, strlen(m.eth_dev_s)+1);

	if(m.sock_raw < 0) {
		perror("socket error");
		return 1;
	}

	uint32_t eth_ip =     *((uint32_t*)&m.eth_ip);
	uint32_t gateway_ip = *((uint32_t*)&m.gate_ip);

	// add gateway to blacklist if needed
	bool allow_gateway = false;
	if(m.allow_gateway_s) {
		allow_gateway = the_great_bool_destringifier(m.allow_gateway_s);
	}

	if(!allow_gateway) {
		arr_add(m.source_blacklist, gateway_ip);
	}

	printf("Listen for ARP requests from Source IPs ");
	print_ip(eth_ip&m.subnet);
	printf(" - ");
	print_ip(eth_ip|~m.subnet);
	
	// Show allowlist information
	if(arr_count(m.source_allowlist) != 0) {
		printf("\nAllowlist (only these sources will trigger wake-up):");
		for(size_t i = 0; i < m.include_dns_cache.count; i++) {
			if(m.include_dns_cache.entries[i].entry) {
				printf(" %s", m.include_dns_cache.entries[i].entry);
				if(m.include_dns_cache.entries[i].ip != 0) {
					printf("(");
					print_ip(m.include_dns_cache.entries[i].ip);
					printf(")");
				}
			}
		}
	}
	
	// Show blocklist information  
	if(arr_count(m.source_blacklist) != 0) {
		printf("\nBlocklist (these sources will be ignored):");
		for(size_t i = 0; i < m.exclude_dns_cache.count; i++) {
			if(m.exclude_dns_cache.entries[i].entry) {
				printf(" %s", m.exclude_dns_cache.entries[i].entry);
				if(m.exclude_dns_cache.entries[i].ip != 0) {
					printf("(");
					print_ip(m.exclude_dns_cache.entries[i].ip);
					printf(")");
				}
			}
		}
		// Also show any IPs added directly (like gateway)
		for(size_t i = m.exclude_dns_cache.count; i < arr_count(m.source_blacklist); i++) {
			printf(" ");
			print_ip(m.source_blacklist[i]);
		}
	}
	
	puts("");
	fflush(stdout); //to see this message in systemctl status

	return 0;
}

int watch_packets() {
	int saddr_size, data_size;
	struct sockaddr saddr;

	while(m.alive) {
		saddr_size = sizeof saddr;
		// receive a packet
		data_size = recvfrom(m.sock_raw, m.buffer, 65536, 0, &saddr, (socklen_t*)&saddr_size);
		if(data_size < 0) {
			if(!m.alive) {
				return 0; //don't print errors for stop
			}
			perror("recvfrom failed to get packets");
			return 1;
		}
		// now process the packet
		RETONFAIL(process_packet(m.buffer));
	}
	return 0;
}

int process_packet(unsigned char* buffer) {
	// get the IP Header part of this packet, excluding the ethernet header
	//struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));

	// TODO: Research packet types for ARP
	// Known types are: 157 (from router), 87 and 129
	//printf("%u\n", iph->protocol);

	// for now, accept all packets
	RETONFAIL(parse_ethhdr(buffer));

	return 0;
}

int parse_arp(unsigned char *data) {
	ns_arp_packet_hdr_t *arp_hdr = (ns_arp_packet_hdr_t *) data;
	ns_arp_IPv4_eth_packet_t *arp_IPv4 = NULL;

	if(ntohs(arp_hdr->ns_arp_hw_type) != NS_ARP_ETHERNET_TYPE) {
		fprintf(stderr, "dis not ethernet :(\n");
		exit(EXIT_FAILURE);
	}

	if(ntohs(arp_hdr->ns_arp_proto_type) != NS_ETH_TYPE_IPv4) {
		fprintf(stderr, "i bet you're using IPv4\n");
		exit(EXIT_FAILURE);
	}

	arp_IPv4 = (ns_arp_IPv4_eth_packet_t *) data;

	// sender and target hardware
	//unsigned char *sh = arp_IPv4->ns_arp_sender_hw_addr;
	//unsigned char *th = arp_IPv4->ns_arp_sender_hw_addr;

	// ARP type
	uint16_t type = ntohs(arp_IPv4->ns_arp_hdr.ns_arp_opcode);

	if(type == NS_ARP_REQUEST) {
		unsigned int eth_ip = *((unsigned int*)&m.eth_ip);
		unsigned int src_ip, ta_ip;
		memcpy(&src_ip, arp_IPv4->ns_arp_sender_proto_addr, sizeof(unsigned int));
		memcpy(&ta_ip, arp_IPv4->ns_arp_target_proto_addr, sizeof(unsigned int));

		if((eth_ip&m.subnet) == (src_ip&m.subnet)) {
			// Non-blocking DNS cache refresh (only if really needed)
			refresh_dns_cache_if_needed_nonblocking(&m.exclude_dns_cache);
			refresh_dns_cache_if_needed_nonblocking(&m.include_dns_cache);
			
			// Check blocklist (source_exclude) first
			bool blocked = false;
			for (size_t i = 0; i < m.exclude_dns_cache.count; ++i) {
				if (m.exclude_dns_cache.entries[i].ip != 0 && m.exclude_dns_cache.entries[i].ip == src_ip) {
					blocked = true;
					break;
				}
			}
			if (blocked) {
				if (m.debug) {
					printf("Blocked '");
					print_ip(src_ip);
					puts("' from the blacklist!");
				}
				return 0;
			}

			// If allowlist is set, only allow if in allowlist
			bool allowed = (m.include_dns_cache.count == 0);
			if (!allowed) {
				for (size_t i = 0; i < m.include_dns_cache.count; ++i) {
					if (m.include_dns_cache.entries[i].ip != 0 && m.include_dns_cache.entries[i].ip == src_ip) {
						allowed = true;
						break;
					}
				}
			}
			if (!allowed) {
				if (m.debug) {
					printf("Source '");
					print_ip(src_ip);
					puts("' not in allowlist!");
				}
				return 0;
			}

			for(size_t i = 0; i < arr_count(m.target_list); i++) {
				struct target *link = &m.target_list[i];
				if(*(unsigned int*)link->ip != ta_ip)
					continue;
				RETONFAIL(send_magic_packet(link->magic));
				printf("Magic packet to '");
				print_ip(ta_ip);
				printf("' sent by '");
				print_ip(src_ip);
				puts("'");
				fflush(stdout);
				break;
			}
		}
	}
	return 0;
}

int parse_ethhdr(unsigned char* buffer) {
	struct ethhdr *eth = (struct ethhdr *)buffer;

	// convert network-endianess to native endianess
	unsigned short eth_protocol = ntohs(eth->h_proto);

	if(eth_protocol == 0x0806) {
		unsigned char* arphdr = buffer + sizeof(struct ethhdr);
		RETONFAIL(parse_arp(arphdr));
	}
	return 0;
}

int read_args(int argc, char *argv[]) {
	for(int i=1; i<argc; i++) {
		if(!strcmp(argv[i], "-h")||!strcmp(argv[i], "--help")) {
			puts(USAGE_INFO);
			return 0;
		} else if(!strcmp(argv[i], "-i")) {
			FAILONARGS(i, argc);
			target_ip_add(m.target_list, 0, strdup(argv[i+1]));
			i++;
		} else if(!strcmp(argv[i], "-m")) {
			FAILONARGS(i, argc);
			target_mac_add(m.target_list, 0, strdup(argv[i+1]));
			i++;
		} else if(!strcmp(argv[i], "-d")) {
			FAILONARGS(i, argc);
			m.eth_dev_s = argv[i+1];
			i++;
		} else if(!strcmp(argv[i], "-b")) {
			FAILONARGS(i, argc);
			m.broadcast_ip_s = argv[i+1];
			i++;
		} else if(!strcmp(argv[i], "-s")) {
			FAILONARGS(i, argc);
			m.subnet_s = argv[i+1];
			i++;
		} else if(!strcmp(argv[i], "-ag")) {
			m.allow_gateway_s = "true";
		} else if(!strcmp(argv[i], "--debug")) {
			m.debug = true;
		}
	}
	return 0;
}

int parse_args() {
	if(!m.eth_dev_s) {
		fprintf(stderr, "Ethernet device to record traffic from not specified!\n");
		return 1;
	}
	if(!m.broadcast_ip_s) {
		fprintf(stderr, "Broadcast IP not specified!\n");
		return 1;
	}
	if(!m.subnet_s) {
		printf("No search subnet provided. Assuming default host IP must match ARP request.\n");
		m.subnet = 0xffffffff;
	}

	// create target macs, ips and magic packets
	RETONFAIL(targets_configure(m.target_list));

	// subnet mask
	if(m.subnet_s) {
		int mask_value;
		int error = sscanf(m.subnet_s, "%d", &mask_value);
		if(error != 1 || mask_value < 0 || mask_value > 31) {
			fprintf(stderr, "Error: Subnet mask must be a value between 0 and 31\n");
		}

		// calculate proper net mask
		unsigned int subnet_bigendian = 0xffffffff << (32-mask_value);
		m.subnet = __builtin_bswap32(subnet_bigendian);
	}

	return 0;
}

int send_magic_packet(unsigned char *magic_packet) {
	int udpSocket = 1;
	int broadcast = 1;
	struct sockaddr_in udpClient, udpServer;

	// setup broadcast socket
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
		perror("socket error");
		return 1;
	}

	// set parameters
	udpClient.sin_family = AF_INET;
	udpClient.sin_addr.s_addr = INADDR_ANY;
	udpClient.sin_port = 0;

	// bind socket
	bind(udpSocket, (struct sockaddr*) &udpClient, sizeof(udpClient));

	// set server end point (the broadcast address)
	udpServer.sin_family = AF_INET;
	udpServer.sin_addr.s_addr = inet_addr(m.broadcast_ip_s);
	udpServer.sin_port = htons(9);

	// set server end point
	sendto(udpSocket, magic_packet, sizeof(unsigned char)*102, 0, (struct sockaddr*) &udpServer, sizeof(udpServer));

	// clean after use
	close(udpSocket);

	return 0;
}

int get_local_ip() {
	int fd;
	struct ifreq ifr;

	// open socket
	fd = socket(AF_INET, SOCK_DGRAM, 0);

	if(fd < 0) {
		perror("socket error");
		return 1;
	}

	// get a IPv4 address specifically
	ifr.ifr_addr.sa_family = AF_INET;

	// get address for the following network device
	strncpy(ifr.ifr_name, m.eth_dev_s, IFNAMSIZ-1);

	// go fetch
	int error = ioctl(fd, SIOCGIFADDR, &ifr);
	if(error == -1) {
		perror("ioctl error");
		return 1;
	}

	// clean up
	close(fd);

	// get the darn address
	m.eth_ip_s = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

	// convert IP back to binary
	sscanf(m.eth_ip_s, "%hhu.%hhu.%hhu.%hhu", &m.eth_ip[0],
						&m.eth_ip[1], &m.eth_ip[2], &m.eth_ip[3]);
	return 0;
}

int load_config() {
	FILE *fp = fopen(CONFIG_PREFIX"/wake-on-arp.conf", "r");
	if(!fp) {
		fprintf(stderr, "Could not open config file: "CONFIG_PREFIX"/wake-on-arp.conf\n");
		// Still initialize arrays to avoid segfaults
		arr_init(m.source_blacklist);
		arr_init(m.source_allowlist);
		arr_init(m.target_list);
		return 0; // Not an error, just skip config loading
	}

	// init variables
	arr_init(m.source_blacklist);
	arr_init(m.source_allowlist);
	arr_init(m.target_list);

	char *line = NULL;
	size_t len;
	while(getline(&line, &len, fp) != -1) {
		char *name, *val;
		int error = sscanf(line, "%ms %ms", &name, &val);
		if(error != 2) continue;

		if(!strcmp("broadcast_ip", name)) {
			m.broadcast_ip_s = val;
		} else if(!strcmp("net_device", name)) {
			m.eth_dev_s = val;
		} else if(!strcmp("subnet", name)) {
			m.subnet_s = val;
		} else if(!strcmp("allow_gateway", name)) {
			m.allow_gateway_s = val;
		} else if(!strncmp("target_mac", name, 10)) {
			unsigned int number = 0;
			if(!sscanf(name, "target_mac_%u", &number)) {
				fprintf(stderr, "Invalid option '%s', should be like 'target_mac_1' (fxp)", name);
				return 2;
			}
			target_mac_add(m.target_list, number, val);
		} else if(!strncmp("target_ip", name, 9)) {
			unsigned int number = 0;
			if(!sscanf(name, "target_ip_%u", &number)) {
				fprintf(stderr, "Invalid option '%s', should be like 'target_ip_1' (fxp)", name);
				return 2;
			}
			target_ip_add(m.target_list, number, val);
		} else if(!strcmp("source_exclude", name)) {
			// Accept either IP or hostname
			arr_add(m.source_blacklist, 0); // Placeholder, will resolve later
			if (m.exclude_dns_cache.count < MAX_SOURCE_LIST_ENTRIES) {
				m.exclude_dns_cache.entries[m.exclude_dns_cache.count].entry = strdup(val);
				m.exclude_dns_cache.entries[m.exclude_dns_cache.count].is_hostname = (strchr(val, '.') == NULL || strspn(val, "0123456789.") != strlen(val));
				m.exclude_dns_cache.count++;
			}
			free(val);
		} else if(!strcmp("source_include", name)) {
			// Accept either IP or hostname
			arr_add(m.source_allowlist, 0); // Placeholder, will resolve later
			if (m.include_dns_cache.count < MAX_SOURCE_LIST_ENTRIES) {
				m.include_dns_cache.entries[m.include_dns_cache.count].entry = strdup(val);
				m.include_dns_cache.entries[m.include_dns_cache.count].is_hostname = (strchr(val, '.') == NULL || strspn(val, "0123456789.") != strlen(val));
				m.include_dns_cache.count++;
			}
			free(val);
		} else free(val); // not used

		// free unused strings
		free(name);
		// WARN: if reload is ever implemented, this is a memory leak
	}
	if(line) free(line);

	fclose(fp);
	return 0;
}

int main(int argc, char *argv[]) {
	DEBUG_PRINT("Starting wake-on-arp with debug mode\n");
	
	// Initialize all fields to zero/NULL for safety
	memset(&m, 0, sizeof(m));
	
	DEBUG_PRINT("Initialized main struct\n");
	
	m.allow_gateway_s = NULL; // init config in case it won't be set
	m.dns_cache_refresh_interval = DEFAULT_DNS_CACHE_REFRESH_INTERVAL;
	m.exclude_dns_cache.count = 0;
	m.include_dns_cache.count = 0;
	m.exclude_dns_cache.last_refresh = 0;
	m.include_dns_cache.last_refresh = 0;
	
	DEBUG_PRINT("Starting config load\n");
	
	// priority: load_config < read_args
	load_config();
	
	DEBUG_PRINT("Config loaded, reading args\n");
	
	RETONFAIL(read_args(argc, argv));
	
	DEBUG_PRINT("Args read, parsing\n");
	
	RETONFAIL(parse_args());
	
	DEBUG_PRINT("Args parsed, refreshing DNS cache\n");
	
	// Initial DNS cache population
	refresh_dns_cache_if_needed(&m.exclude_dns_cache);
	refresh_dns_cache_if_needed(&m.include_dns_cache);
	
	DEBUG_PRINT("DNS cache refreshed, initializing\n");
	
	RETONFAIL(initialize());
	
	DEBUG_PRINT("Initialized, starting packet watch\n");
	
	RETONFAIL(watch_packets());
	
	DEBUG_PRINT("Cleaning up\n");
	
	cleanup();
	return 0;
}


