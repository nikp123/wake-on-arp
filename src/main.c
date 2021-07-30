#include <errno.h>
#include <netdb.h>
#include <stdio.h>		// for standard things
#include <stdlib.h>		// malloc
#include <string.h>		// strlen
#include <stdbool.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>				// provides declarations for ip header
#include <netinet/if_ether.h>		// for ETH_P_ALL
#include <net/ethernet.h>			// for ether_header
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "ns_arp.h"
#include "ns_arp_packet.h"

#include "functions.h"

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
"For further info look here: https://github.com/nikp123/wake-on-arp/issues/1#issuecomment-882708765\n";

void cleanup();
void sig_handler();
int initialize();
int watch_packets();
int process_packet(unsigned char*);
int parse_arp(unsigned char *);
int parse_ethhdr(unsigned char*);
int get_local_ip();
int send_magic_packet(unsigned char*);

struct main {
	char *eth_dev_s;
	char *eth_ip_s;
	char *broadcast_ip_s;
	char *subnet_s;
	char *allow_gateway_s;

	struct target *target_linked_list;

	unsigned char eth_ip[4];
	unsigned char gate_ip[4];

	unsigned int  subnet;

	bool allow_gateway;

	unsigned char *buffer;
	int sock_raw;
	bool alive;
} m;

void cleanup() {
	destroy_linked_list(m.target_linked_list);
	close(m.sock_raw);
	free(m.buffer);
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
	
	unsigned int eth_ip = *((unsigned int*)&m.eth_ip);
	unsigned int gateway_ip = *((unsigned int*)&m.gate_ip);

	printf("Listen for ARP requests from Source IPs ");
	print_ip(eth_ip&m.subnet);
	printf(" - ");
	print_ip(eth_ip|~m.subnet);
	if(!m.allow_gateway) {
		printf(" but ignore ");
		print_ip(gateway_ip);
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
		// if source matches to host
		// and if target matches send magic
		unsigned int eth_ip = *((unsigned int*)&m.eth_ip);
		unsigned int gateway_ip = *((unsigned int*)&m.gate_ip);

		// sender and target address
		unsigned int src_ip, ta_ip;
		memcpy(&src_ip, arp_IPv4->ns_arp_sender_proto_addr, sizeof(unsigned int));
		memcpy(&ta_ip, arp_IPv4->ns_arp_target_proto_addr, sizeof(unsigned int));

		if((eth_ip&m.subnet) == (src_ip&m.subnet)) {
			struct target *link = m.target_linked_list;
			for(; link; link=link->next) {
				if(*(unsigned int*)link->ip != ta_ip)
					continue;

				if(!m.allow_gateway) {
					if(src_ip == gateway_ip) {
						#ifdef DEBUG
							puts("Blocked ARP wake-up by gateway");
							fflush(stdout);
						#endif
						return 0;
					}
				}
				RETONFAIL(send_magic_packet(link->magic));
				printf("Magic packet to '");
				print_ip(ta_ip);
				printf("' sent by '");
				print_ip(src_ip);
				puts("'");
				fflush(stdout); // Write now to get an accurate timestamp for analyzing wake-up reason
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
			add_ip_to_linked_list(&m.target_linked_list, 0, argv[i+1]); 
			i++;
		} else if(!strcmp(argv[i], "-m")) {
			FAILONARGS(i, argc);
			add_mac_to_linked_list(&m.target_linked_list, 0, argv[i+1]); 
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
	RETONFAIL(check_linked_list(m.target_linked_list));

	// subnet mask
	if(m.subnet_s) {
		int mask_value;
		int error = sscanf(m.subnet_s, "%d", &mask_value);
		if(error != 1 || mask_value < 0 || mask_value > 31) {
			fprintf(stderr, "Error: Subnet mask must be a value between 0 and 31\n");
		}

		// calculate proper net mask
		m.subnet = 0xffffffff >> (32-mask_value);  
	}

	// should we allow gateway ARP requests
	m.allow_gateway = false;
	if(m.allow_gateway_s) {
		m.allow_gateway = the_great_bool_destringifier(m.allow_gateway_s);
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
		return 1;
	}

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
			add_mac_to_linked_list(&m.target_linked_list, number, val);
		} else if(!strncmp("target_ip", name, 9)) {
			unsigned int number = 0;
			if(!sscanf(name, "target_ip_%u", &number)) {
				fprintf(stderr, "Invalid option '%s', should be like 'target_ip_1' (fxp)", name);
				return 2;
			}
			add_ip_to_linked_list(&m.target_linked_list, number, val);
		} else free(val); // not used

		// free unused strings
		free(name);
		// WARN: if reload is ever implemented, this is a memory leak
	}
	if(line) free(line);

	// weird seg. fault on ARMv7 (have to investigate)
	//fclose(fp);
	return 0;
}

int main(int argc, char *argv[]) {
	m.allow_gateway_s = NULL; // init config in case it won't be set
	// priority: load_config < read_args
	load_config();
	RETONFAIL(read_args(argc, argv));
	RETONFAIL(parse_args());
	RETONFAIL(initialize());
	RETONFAIL(watch_packets());
	cleanup();
	return 0;
}


