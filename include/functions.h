#include <sys/socket.h>
#include <stdbool.h>

#include "data-types.h"

int get_gateway_ip(unsigned char *gateway_ip, char *net_interface);
bool the_great_bool_destringifier(const char *s);
void print_ip(unsigned int ip);

void target_ip_add(struct target *list, unsigned int id, char *string);
void target_mac_add(struct target *list, unsigned int id, char *string);
int  targets_configure(struct target *list);
void targets_destroy(struct target *list);
