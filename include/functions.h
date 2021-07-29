#include <sys/socket.h>
#include <stdbool.h>

#include "data-types.h"

int get_gateway_ip(unsigned char *gateway_ip, char *net_interface);
bool the_great_bool_destringifier(const char *s);
void print_ip(unsigned int ip);


int check_linked_list(struct target *list);
void destroy_linked_list(struct target *list);
void add_ip_to_linked_list(struct target **linked_list, unsigned int id, char *string);
void add_mac_to_linked_list(struct target **linked_list, unsigned int id, char *string);
