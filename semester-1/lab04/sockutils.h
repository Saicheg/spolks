#ifndef SOCKUTILS_H
#define SOCKUTILS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int mksock(char * host,
        char * service,
        char * proto,
        struct sockaddr_in * sin);


int servsock(char * host,
             char * service,
             char * proto,
             struct sockaddr_in * sin,
             int qlen);

#endif/* SOCKUTILS_H */

