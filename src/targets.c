#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "data-types.h"

void create_magic_packet(struct target *link) {
	// 6 x 0xFF on start of packet
	memset(link->magic, 0xFF, 6);

	// rest are just copies of the MAC address
	for(int i = 1; i <= 16; i++) {
		memcpy(&link->magic[i*6], &link->mac, 6*sizeof(unsigned char));
	}
}

// retuns offset in the linked lsit array to the apropriate id
struct target *find_or_create_id(struct target **linked_list, unsigned int id) {
	if(*linked_list == NULL) {
		*linked_list = calloc(1, sizeof(struct target));
		(*linked_list)[0].id = id;
		return (*linked_list);
	} 

	unsigned int i = 0;
	struct target *link = (*linked_list);
	struct target *last_link = 0;
	while(link) {
		if(link->id == id)
			return link;
		else {
			last_link = link;
			link = link->next;
			i++;
		}
	}

	// if we've managed to break out of that loop without returning, that means that the id doesn't exist
	link = last_link;
	link->next = calloc(1, sizeof(struct target));
	link = link->next;
	link->id = id;
	return link;
}

void add_mac_to_linked_list(struct target **linked_list, unsigned int id, char *string) {
	struct target *element = find_or_create_id(linked_list, id);
	element->mac_s = string;
}

void add_ip_to_linked_list(struct target **linked_list, unsigned int id, char *string) {
	struct target *element = find_or_create_id(linked_list, id);
	element->ip_s = string;
}

int check_linked_list(struct target *list) {
	struct target *ta = list;
	while(ta != NULL) {
		if(ta->mac_s == NULL) {
			fprintf(stderr, "Missing element 'target_mac_%u'\n",
					ta->id);
			return 1;
		}
		if(ta->ip_s == NULL) {
			fprintf(stderr, "Missing element 'target_ip_%u'\n",
					ta->id);
			return 1;
		}

		// device MAC
		int error = sscanf(ta->mac_s, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx", &ta->mac[0], &ta->mac[1],
										&ta->mac[2], &ta->mac[3], &ta->mac[4], &ta->mac[5]);
		// maybe the user typed it uppercase?
		if(error != 6) {
			error = sscanf(ta->mac_s, "%2hhX:%2hhX:%2hhX:%2hhX:%2hhX:%2hhX",
				&ta->mac[0], &ta->mac[1], &ta->mac[2], &ta->mac[3], &ta->mac[4], &ta->mac[5]);
		}
		if(error != 6) {
			fprintf(stderr, "Invalid 'ta_mac_%u' address: \"%s\"\n", ta->id, ta->mac_s);
			return 2;
		}

		// device IP
		error = sscanf(ta->ip_s, "%hhu.%hhu.%hhu.%hhu", &ta->ip[0],
									&ta->ip[1], &ta->ip[2], &ta->ip[3]);
		if(error != 4) {
			fprintf(stderr, "Invalid 'ta_ip_%u' address: \"%s\"\n", ta->id, ta->ip_s);
			return 2;
		}

		create_magic_packet(ta);
		ta = ta->next;
	}
	return 0;
}

void destroy_linked_list(struct target *list) {
	struct target *next = list;

	// jump to the end of the linked list
	while(next) {
		list = next;
		next = list->next;
		list->next = NULL; // invalidate the pointer because my shit code
		free(list);
	}
}

