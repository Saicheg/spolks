#include "sockutils.h"
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
/* #include <sys/sendfile.h> */
#include <unistd.h>
#include <fcntl.h>
#define BUFF_SIZE 1024

int main(int argc, char* argv[]) {
  if(argc < 3){
    perror("Неправильный формат данных: <HOST> <PORT>");
    exit(EXIT_FAILURE);
  }

  char* filename="test/test"; // keep filename static for test purposes
  char *host = argv[1], *service = argv[2], *proto = "tcp";
  struct sockaddr_in sin, remote;
  int sd, desc, n;
  socklen_t rlen;
  char buffer[BUFF_SIZE];
  char oob_buffer[BUFF_SIZE];
  FILE* fd;

  if ( (sd = servsock(host, service, proto,  &sin, 10)) == -1) {
    printf( "Ошибка при создании сокета\n");
    return 1;
  }

  while(1) {
    rlen = sizeof(remote);
    desc = accept(sd, (struct sockaddr*) &remote, &rlen);
    if (desc == -1) {
      printf("Ошибка при подключении\n");
      continue;
    }

    fprintf(stderr, "Начало передачи файла: %s\n", filename);

    fd = fopen(filename, "r");
    if(fd == (FILE*)NULL) {
      printf("Невозможно прочитать файл\n");
    }

    while(1){
      n = recv(desc, oob_buffer, sizeof(oob_buffer), MSG_OOB);
      if(n > 0) {
        fputs(oob_buffer, stdout);
      }
      bzero(buffer, BUFF_SIZE);
      fread(buffer, sizeof(char), BUFF_SIZE, fd);
      n = send(desc, buffer, BUFF_SIZE, 0);
      if (n < 0) {
        printf("Ошибка при записи в сокет\n");
        break;
      }
      if(feof(fd)) {
        break;
      }
    }

    fprintf(stderr, "Конец передачи файла: %s\n", filename);

    fclose(fd); // file descriptor
    close(desc); // client descriptor
  }
  close(sd);
  return 0;
}
