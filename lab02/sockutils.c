#include "sockutils.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int mksock(char *host, char * service, char * proto, struct sockaddr_in *sin)
{
  struct hostent *hptr;
  struct servent *sptr;
  struct protoent *pptr;
  int sd = 0, type;

  memset(sin, 0, sizeof ( *sin));
  sin->sin_family = AF_INET;

  if (hptr = gethostbyname(host))
    memcpy(& sin->sin_addr, hptr->h_addr, hptr->h_length);
  else return -1;

  if (!(pptr = getprotobyname(proto))) return -1;

  if (sptr = getservbyname(service, proto))
    sin->sin_port = sptr->s_port;
  else
    if ((sin->sin_port = htons((unsigned short) atoi(service))) == 0) return -1;

  if (strcmp(proto, "udp") == 0)
    type = SOCK_DGRAM;
  else
    type = SOCK_STREAM;

  if ((sd = socket(PF_INET, type, pptr->p_proto)) < 0) {
    perror("Socket allocation error.");
    return -1;
  }
  return sd;
}

int servsock(char *host, char * service, char * proto, struct sockaddr_in *sin)
{
  int sd;
  if ((sd = mksock(host, service, proto, (struct sockaddr_in *) sin)) == -1)
    return -1;
  if (bind(sd, (struct sockaddr *) sin, sizeof ( *sin)) < 0) {
    perror("Error binding socket");
    return -1;
  }

  if (strcmp(proto, "tcp") == 0) {
    if (listen(sd, qlen) == -1) {
      perror("Error switching socket to passive mode");
      return -1;
    }
  }
  return sd;
}