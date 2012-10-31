#include "sockutils.h"
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
  if(argc < 3){
    perror("wrong params format: <HOST> <PORT>");
    exit(EXIT_FAILURE);
  }

  char* filename="test/test"; // keep filename static for test purposes
  char *host = argv[1], *service = argv[2], *proto = "tcp";
  struct sockaddr_in sin, remote;
  struct stat stat_buf;      /* argument to fstat */
  off_t offset = 0;          /* file offset */
  int sd, rlen, desc, fd, rc;
  char buf[1024];

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

    fd = open(filename, O_RDONLY);
    if(fd == -1) {
      printf("Невозможно прочитать файл\n");
      continue;
    }

    fstat(fd, &stat_buf);
    offset = 0;

    fprintf(stderr, "Start sending file %s\n", filename);
    rc = sendfile (desc, fd, &offset, stat_buf.st_size);
    if (rc == -1) {
      fprintf(stderr, "error from sendfile: %s\n", strerror(errno));
      continue;

    }
    if (rc != stat_buf.st_size) {
      fprintf(stderr, "incomplete transfer from sendfile: %d of %d bytes\n", rc, (int)stat_buf.st_size);
      continue;
    }
    fprintf(stderr, "End sending file %s\n", filename);

    close(fd); // file descriptor
    close(desc); // client descriptor
  }
  close(sd);
  return 0;
}
