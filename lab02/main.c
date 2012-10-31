#include "sockutils.h"

int main() {
  char *host = "localhost", *service = "2525", *proto = "tcp";
  struct sockaddr_in sin, remote;
  struct timeval timev;
  int sd, rsd, rlen, readed;
  char buf[513], *t_now;

  if ( (sd = servsock(host, service, proto,  &sin, 10)) == -1) {
    printf( "Ошибка при создании сокета\n");
    return 1;
  }

  while(1) {
    rlen = sizeof(remote);
    rsd = accept(sd, (struct sockaddr*) &remote, &rlen);
    if( (readed = recv(rsd, buf, 512, 0)) != -1) {
      printf("Test");
      gettimeofday( &timev, NULL);
      t_now = ctime( &(timev.tv_sec));
      send(rsd, t_now, strlen(t_now), 0);
    }
    close(rsd);
  }
  return 0;
}
