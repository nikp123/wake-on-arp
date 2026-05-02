#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "array.h"
#include "data-types.h"

void target_generate_magic(struct target *link) {
	// 6 x 0xFF on start of packet
	memset(link->magic, 0xFF, 6);

	// rest are just copies of the MAC address
	for(int i = 1; i <= 16; i++) {
		memcpy(&link->magic[i*6], &link->mac, 6*sizeof(unsigned char));
	}
}

void target_mac_add(struct target *list, unsigned int id, char *string) {
	if(id >= arr_count(list)) {
		// the hope is that new targets have "populated" unset
		arr_resize_zero(list, id+1);
	}

	list[id].populated = true;
	list[id].mac_s = string;
}

void target_ip_add(struct target *list, unsigned int id, char *string) {
	if(id >= arr_count(list)) {
		// the hope is that new targets have "populated" unset
		arr_resize_zero(list, id+1);
	}

	list[id].populated = true;
	list[id].ip_s = string;
}

int targets_configure(struct target *list) {
	for(size_t i = 0; i < arr_count(list); i++) {
		if(!list[i].populated) // skip unpopulated ids
			continue;

		struct target *ta = &list[i];
		if(ta->mac_s == NULL) {
			fprintf(stderr, "Missing element 'target_mac_%zu'\n", i);
			return 1;
		}
		if(ta->ip_s == NULL) {
			fprintf(stderr, "Missing element 'target_ip_%zu'\n", i);
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
			fprintf(stderr, "Invalid 'ta_mac_%zu' address: \"%s\"\n", i, ta->mac_s);
			return 2;
		}

		// device IP
		error = sscanf(ta->ip_s, "%hhu.%hhu.%hhu.%hhu", &ta->ip[0],
									&ta->ip[1], &ta->ip[2], &ta->ip[3]);
		if(error != 4) {
			fprintf(stderr, "Invalid 'ta_ip_%zu' address: \"%s\"\n", i, ta->ip_s);
			return 2;
		}

		target_generate_magic(ta);
	}

	return 0;
}

void targets_destroy(struct target *list) {
	for(size_t i = 0; i < arr_count(list); i++) {
		//ip_s and mac_s need a seperate free call because they was malloced with sscanf
		free(list[i].ip_s);
		free(list[i].mac_s);
	}
	arr_free(list);
}

