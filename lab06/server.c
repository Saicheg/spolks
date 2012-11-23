#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include "sockutils.h"
#include "common.h"

void tcp_server(int sd);
void udp_server(int sd);

char *host, *service, *proto, *filename;
FILE* fd;
struct sockaddr_in server_addr, remote_addr;
socklen_t rlen;
fd_set rfds, afds;

int main(int argc, char* argv[]) {
  int sd;

  proto = argv[1];
  host = argv[2];
  service = argv[3];
  filename = argv[4];


  if(argc < 5){
    printf("Params format: <tcp|udp> <HOST> <PORT> <FILENAME>\n");
    exit(EXIT_SUCCESS);
  }

  if ( (sd = servsock(host, service, proto,  &server_addr, 10)) == -1) {
    perror("\nError creating socket: ");
    exit(EXIT_FAILURE);
  }

  printf("Server started...\n");

  if (strcmp(proto, "udp") == 0) {
    udp_server(sd);
  } else {
    tcp_server(sd);
  }

  close(sd);
  return 0;
}

void tcp_server(int sd) {

  off_t offset = 0;          /* file offset */
  int i, j, k, desc, maxfd, maxi, client[FD_SETSIZE], numready;
  char buf[BUF_SIZE];

  maxfd = sd;
  maxi = -1;

  for (i=0; i < FD_SETSIZE; i++) {
    client[i] = -1;
  }

  FD_ZERO(&afds);
  FD_ZERO(&rfds);
  FD_SET(sd, &afds);


  while(1) {
    rlen = sizeof(struct sockaddr_in);

    memcpy( &rfds, &afds, sizeof(rfds) );


    numready = select( maxfd +1 , &rfds, (fd_set*) 0,(fd_set*) 0, (struct timeval*) 0);
    if(numready == -1) {
      perror("\nServer select error: ");
      exit(EXIT_FAILURE);
    }

    if(FD_ISSET(sd, &rfds)) {

      desc = accept(sd, (struct sockaddr*) &remote_addr, &rlen);
      if (desc == -1) {
        perror("\nConnection error: ");
        continue;
      }

      for(j=0; j<FD_SETSIZE; j++) {

        if(client[j] < 0) {
          client[j] = desc;
          break;
        }

        FD_SET(desc, &afds);

        if(desc > maxfd) {
          maxfd = desc;
        }

        if(j > maxi) {
          maxi = j;
        }
      }

    }

    for(k = 0; k <= maxi; k++) {

      if(client[k] > 0) {
        if(FD_ISSET(client[k], &afds)) {

          // Read file start offset
          if(read(client[k], buf, sizeof(buf)*sizeof(buf[0])) > 0 ) {
            offset = atoi(buf);
            printf("Requested start offset: %lu\n", offset);
          } else {
            perror("\nError getting start file offset: ");
            close(client[k]);
            FD_CLR(client[k], &afds);
/*             client[k] = -1; */
            fclose(fd);
            continue;
          }

          fd = fopen(filename, "r");
          if(fd == NULL) {
            perror("\nError opening file: ");
            close(sd);
            exit(EXIT_FAILURE);
          }
          fseek(fd, offset, SEEK_SET);


          size_t bytes_total = 0;
          printf("Start sending file %s\n", filename);

          while(1){

            size_t bytes_read = fread(buf, sizeof(buf[0]), sizeof(buf), fd);
            size_t bytes_sent = send(client[k], buf, bytes_read, 0);
            if (bytes_sent == -1) {
              perror("\nError writing to socket: ");
              break;
            }

            bytes_total += bytes_sent;
            printf("Sent last/total: %zd/%zd\n", bytes_sent, bytes_total);

            /* bytes_sent = send(desc, oob_message, 1, MSG_OOB); */
            /* if (bytes_sent == -1) */
            /* { */
            /*   perror("\nError sending out-of-band data: "); */
            /* } */


            if(feof(fd)) {
              printf("End sending file %s\n", filename);
              break;
            }
          }

          close(client[k]);
          FD_CLR(client[k], &afds);
/*           client[k] = -1; */
          fclose(fd); // file descriptor
        }
      }
      if(--numready <= 0) break;
    }
  }
}

void udp_server(int sd) {

  off_t offset = 0;          /* file offset */
  int num;
  char buf[BUF_SIZE];
  struct stat st;
  long long size;
  size_t bytes_read, bytes_sent;

  fd = fopen(filename, "r");
  if(fd == NULL) {
    perror("\nError opening file: ");
    close(sd);
    exit(EXIT_FAILURE);
  }

  stat(filename, &st);
  size = st.st_size;

  while(1) {
    rlen = sizeof(remote_addr);
    num = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr*) &remote_addr, &rlen);

    if(num > 0) {
      offset = atoi(buf);
      printf("Request with offset: %lu\n", offset);
      if(offset < size) {
        fseek(fd, offset, SEEK_SET);
        bytes_read = fread(buf, sizeof(buf[0]), sizeof(buf), fd);
        bytes_sent = sendto(sd, buf, bytes_read, 0, (struct sockaddr *) &remote_addr, rlen);
        if (bytes_sent < 0) {
          perror("\nError sending data: ");
          continue;
        }
      } else {
        bytes_sent = sendto(sd, end_message, sizeof(end_message), 0, (struct sockaddr *) &remote_addr, rlen);
        if (bytes_sent < 0) {
          perror("\nError sending EOF message: ");
          continue;
        }
      }
    }
  }
}
