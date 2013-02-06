#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "sockutils.h"
#include "common.h"

void tcp_server(int sd);
void udp_server(int sd);
void* tcp_server_thread(void * desc);
void* udp_server_thread(void * req);

char *host, *service, *proto, *filename;
struct sockaddr_in server_addr, remote_addr;
socklen_t rlen;

int main(int argc, char* argv[]) {
  int sd;
  int one = 1;

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

  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
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
  pthread_t th;
  //pthread_attr_t ta;

  while(1) {
    int * desc = (int*) calloc(1, sizeof(int));
    rlen = sizeof(struct sockaddr_in);
    desc[0] = accept(sd, (struct sockaddr*) &remote_addr, &rlen);
    if (desc[0] == -1) {
      perror("\nConnection error: ");
      continue;
    }
    pthread_create(&th, NULL, tcp_server_thread, desc);
  }
}

void* tcp_server_thread(void* desc){
  int rsd = *(int*)desc;
  free(desc);
  printf("THREAD: Created thread %zd\n", pthread_self());
  off_t offset = 0;  //file offset
  FILE* fd;
  char buf[BUF_SIZE];

  // Read file start offset
  if(read(rsd, buf, sizeof(buf) * sizeof(buf[0])) == -1 ) {
    perror("\nError getting start file offset: ");
    close(rsd);
    return NULL;
  }
  offset = atoi(buf);
  printf("Requested start offset: %lu\n", offset);

  fd = fopen(filename, "r");
  if(fd == NULL) {
    perror("\nError opening file: ");
    return NULL;
  }
  fseek(fd, offset, SEEK_SET);


  size_t bytes_total = 0;
  printf("Start sending file %s\n", filename);
  while(1){
    size_t bytes_read = fread(buf, sizeof(buf[0]), sizeof(buf), fd);
    size_t bytes_sent = send(rsd, buf, bytes_read, 0);
    if (bytes_sent == -1) {
      perror("\nError writing to socket: ");
      break;
    }

    bytes_total += bytes_sent;
    //printf("Sent last/total: %zd/%zd\n", bytes_sent, bytes_total);

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
  /* send(desc, oob_message, 1, MSG_OOB); */
  shutdown(rsd, SHUT_RDWR);
  fclose(fd); // file descriptor
  close(rsd); // client descriptor
  return NULL;
}

void udp_server(int sd) {
  pthread_t th;
  int num;

  while(1) {
    struct request_udp * req = (struct request_udp*) calloc(1, sizeof(struct request_udp));
    req->rbuflen = sizeof(req->rbuf[0]) * sizeof(req->rbuf);
    req->rlen = sizeof(struct sockaddr_in);
    req->rsock = sd;
    num = recvfrom(sd,
                   req->rbuf,
                   req->rbuflen,
                   0,
                   (struct sockaddr*) &(req->raddr),
                   &req->rlen);
    if(num > 0) {
      pthread_create(&th, NULL, udp_server_thread, req);
    }
  }
}

void* udp_server_thread(void * req) {
  printf("THREAD: Created thread %zd\n", pthread_self());
  struct request_udp * request = (struct request_udp *) req;
  FILE* fd;
  struct stat st;
  size_t size;
  off_t offset = 0;          /* file offset */
  size_t bytes_read, bytes_sent;
  char buf[BUF_SIZE];

  fd = fopen(filename, "r");
  if(fd == NULL) {
    perror("\nError opening file: ");
    free(request);
    return NULL;
  }

  stat(filename, &st);
  size = st.st_size;

  offset = atoi(request->rbuf);
  //printf("Request with offset: %lu\n", offset);
  if(offset < size) {
    fseek(fd, offset, SEEK_SET);
    bytes_read = fread(buf, sizeof(buf[0]), sizeof(buf), fd);
    bytes_sent = sendto(request->rsock, buf, bytes_read, 0, (struct sockaddr *) &(request->raddr), request->rlen);
    if (bytes_sent < 0) {
      perror("\nError sending data: ");
      fclose(fd);
      free(request);
      return NULL;
    }
  } else {
    bytes_sent = sendto(request->rsock, end_message, sizeof(end_message), 0, (struct sockaddr *) &(request->raddr), request->rlen);
    if (bytes_sent < 0) {
      perror("\nError sending EOF message: ");
      fclose(fd);
      free(request);
      return NULL;
    }
  }

  fclose(fd);
  free(request);
  return NULL;
}
