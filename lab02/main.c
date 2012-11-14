#include "sockutils.h"
#include <time.h>
#include <unistd.h>

int main() {
  char *host = "localhost", *service = "9292", *proto = "tcp";
  struct sockaddr_in sin, remote;
  time_t current_time;
  int sd, rsd;
  socklen_t rlen;
  char *t_now;

  if ( (sd = servsock(host, service, proto,  &sin, 10)) == -1) {
    printf( "Ошибка при создании сокета\n");
    return 1;
  }

  while(1) {
    rlen = sizeof(remote);
    rsd = accept(sd, (struct sockaddr*) &remote, &rlen);
    current_time = time(NULL);
    t_now = ctime( &current_time );
    send(rsd, t_now, strlen(t_now), 0);
    close(rsd);
  }
  return 0;
}
