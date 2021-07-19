#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// because casting a string hasn't been invented yet
bool the_great_bool_destringifier(const char *s) {
	if(!strcmp(s, "true")) return true;
	else if(!strcmp(s, "yes")) return true;
	else if(!strcmp(s, "1")) return true;
	else if(!strcmp(s, "y")) return true;
	else if(!strcmp(s, "enable")) return true;
	else if(!strcmp(s, "enabled")) return true;
	else if(!strcmp(s, "false")) return false;
	else if(!strcmp(s, "no")) return false;
	else if(!strcmp(s, "0")) return false;
	else if(!strcmp(s, "n")) return false;
	else if(!strcmp(s, "disable")) return false;
	else if(!strcmp(s, "disabled")) return false;
	else {
		fprintf(stderr, "Invalid bool value of '%s'!\n", s);
	}
	return false;
}

// inet_ntoa?
void print_ip(unsigned int ip) {
	printf("%hhu.%hhu.%hhu.%hhu",
			(unsigned char)(ip&0xff), (unsigned char)(ip>>8&0xff),
			(unsigned char)(ip>>16&0xff),  (unsigned char)(ip>>24&0xff));
}

