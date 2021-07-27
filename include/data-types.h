// implementing something like this is LITERAL HELL which is why im doing linked lists :D
struct target {
	unsigned int id;
	unsigned char ip[4];
	unsigned char mac[6];
	unsigned char magic[102];
	char *ip_s;
	char *mac_s;
	struct target *next;
};

