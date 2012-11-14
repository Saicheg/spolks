#include "sockutils.h"
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if(argc < 4){
    perror("Params format: <HOST> <PORT> <FILE>");
    exit(EXIT_FAILURE);
  }

  char* filename = argv[3];
  char *host = argv[1], *service = argv[2], *proto = "tcp";
  struct sockaddr_in sin, remote;
  struct stat stat_buf;      /* argument to fstat */
  off_t offset = 0;          /* file offset */
  int sd, desc, fd, rc;
  socklen_t rlen;
  char buf[1024];

  if ( (sd = servsock(host, service, proto,  &sin, 10)) == -1) {
    perror("\nError creating socket:");
    return 1;
  }

  printf("Server started...\n");

  while(1) {
    rlen = sizeof(struct sockaddr_in);
    desc = accept(sd, (struct sockaddr*) &remote, &rlen);
    if (desc == -1) {
      perror("\nConnection error: ");
      continue;
    }

    fd = open(filename, O_RDONLY);
    if(fd == -1) {
      perror("\nError opening file: ");
      close(desc);
      continue;
    }

    // Read file length
    if(read(desc, buf, sizeof(buf)) > 0 ) {
      offset = atoi(buf);
      printf("Requested start offset: %lu\n", offset);
    } else {
      perror("\nError getting start file offset: ");
      close(desc);
      continue;
    }

    fstat(fd, &stat_buf);
    fprintf(stderr, "Start sending file %s\n", filename);
    rc = sendfile (desc, fd, &offset, stat_buf.st_size);
    if (rc == -1) {
      perror("\nSendfile error: ");
      continue;
    }

    fprintf(stdout, "End sending file %s\n", filename);

    close(fd); // file descriptor
    close(desc); // client descriptor
  }
  close(sd);
  return 0;
}
