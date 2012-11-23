#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

#define BUF_SIZE 1024



char *oob_message = "~";
char *end_message = "e_o_f";

struct request_udp {
	int rsock;
	socklen_t rlen;
	struct sockaddr_in raddr;
	char rbuf[BUF_SIZE];
	size_t rbuflen;

};

#endif
