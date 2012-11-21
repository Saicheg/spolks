#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>

#include "sockutils.h"
#include "common.h"

int main(int argc, char* argv[]) {
  if(argc < 4){
    printf("Params format: <HOST> <PORT> <FILENAME>\n");
    exit(EXIT_SUCCESS);
  }

  char* filename = argv[3];
  char *host = argv[1], *service = argv[2], *proto = "tcp";
  struct sockaddr_in sin, remote;
  off_t offset = 0;          /* file offset */
  int sd, desc;
  FILE* fd;
  socklen_t rlen;
  char buf[BUF_SIZE];

  if ( (sd = servsock(host, service, proto,  &sin, 10)) == -1) {
    perror("\nError creating socket: ");
    exit(EXIT_FAILURE);
  }

  printf("Server started...\n");

  while(1) {
    rlen = sizeof(struct sockaddr_in);
    desc = accept(sd, (struct sockaddr*) &remote, &rlen);
    if (desc == -1) {
      perror("\nConnection error: ");
      continue;
    }

    fd = fopen(filename, "r");
    if(fd == NULL) {
      perror("\nError opening file: ");
      close(desc);
      continue;
    }

    // Read file start offset
    if(read(desc, buf, sizeof(buf)*sizeof(buf[0])) > 0 ) {
      offset = atoi(buf);
      printf("Requested start offset: %lu\n", offset);
    } else {
      perror("\nError getting start file offset: ");
      close(desc);
      fclose(fd);
      continue;
    }
    fseek(fd, offset, SEEK_SET);


    size_t bytes_total = 0;
    printf("Start sending file %s\n", filename);
    while(1){
      size_t bytes_read = fread(buf, sizeof(buf[0]), sizeof(buf), fd);
      size_t bytes_sent = send(desc, buf, bytes_read, 0);
      if (bytes_sent < 0) {
        perror("\nError writing to socket: ");
        break;
      }

      bytes_total += bytes_sent;
      printf("Sent last/total: %zd/%zd\n", bytes_sent, bytes_total);

      bytes_sent = send(desc, oob_message, 1, MSG_OOB);
      if (bytes_sent == -1)
      {
        perror("\nError sending out-of-band data: ");
      }


      if(feof(fd)) {
        printf("End sending file %s\n", filename);
        break;
      }
    }
    send(desc, oob_message, 1, MSG_OOB);
    shutdown(desc, SHUT_RDWR);
    fclose(fd); // file descriptor
    close(desc); // client descriptor
  }
  close(sd);
  return 0;
}
