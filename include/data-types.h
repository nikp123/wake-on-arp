#ifndef __DATA_TYPES_H
#define __DATA_TYPES_H

#include <stdbool.h>

struct target {
	unsigned char ip[4];
	unsigned char mac[6];
	unsigned char magic[102];
	char *ip_s;
	char *mac_s;
	bool populated;
};

#endif

