#include <sys/socket.h>
#include <stdbool.h>

int get_gateway_ip(unsigned char *gateway_ip, socklen_t size, char *net_interface);
bool the_great_bool_destringifier(const char *s);
void print_ip(unsigned int ip);
