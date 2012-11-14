#include "sockutils.h"
#include <time.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if(argc < 3){
    perror("wrong params format: <HOST> <PORT>");
    return -1;
  }
  char *host = argv[1], *service = argv[2], *proto = "tcp";
  struct sockaddr_in sin;
  int sd, n, count=0, status;
  char buffer[1024];
  char* msg = "msg\n";

  if ((sd = mksock(host, service, proto,  &sin)) == -1) {
    printf("Ошибка при подключении к сокету\n");
    exit(EXIT_FAILURE);
  }

  if (connect(sd, (struct sockaddr *) &sin, sizeof(sin) ) == -1 ) {
    printf("Ошибка при соединении с сервером\n");
    exit(EXIT_FAILURE);
  }

  while ( (n = read(sd, buffer, sizeof(buffer))) > 0 ) {
    count += n;
    if ((status = send(sd, msg, sizeof(msg), MSG_OOB)) < 0) {
      perror("\n   send: ");
    }
    fputs(buffer, stdout);
  }

  if ( n < 0 ) {
    printf("Ошибка при получении\n");
    exit(EXIT_FAILURE);
  }

  close(sd);
  return EXIT_SUCCESS;
}
