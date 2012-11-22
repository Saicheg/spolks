#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include "sockutils.h"
#include "common.h"

size_t bytes_total = 0;
size_t bytes_read= 0;
int sd;
char buffer_oob[2];
struct sockaddr_in sin, remote;
socklen_t rlen;
struct stat st;
FILE* fd;

char *host, *service, *proto, *filename;

void tcp_client(int sd);
void udp_client(int sd);

int main(int argc, char* argv[]) {
  if(argc < 5){
    printf("Params format: <tcp|udp> <HOST> <PORT> <FILENAME>\n");
    exit(EXIT_SUCCESS);
  }

  proto = argv[1];
  host = argv[2];
  service = argv[3];
  filename = argv[4];

  if ((sd = mksock(host, service, proto,  &sin)) == -1) {
    perror("\nError creating socket: ");
    exit(EXIT_FAILURE);
  }

  if (strcmp(proto, "udp") == 0) {
    udp_client(sd);
  } else {
    tcp_client(sd);
  }

  close(sd);
  return EXIT_SUCCESS;
}


void tcp_client(int sd){

  char buffer[BUF_SIZE];

  // Open file and see if it exists
  if( access( filename, F_OK ) != -1 ) {
    stat(filename, &st);
    sprintf(buffer, "%lld", (long long) st.st_size);
  } else {
    strcpy(buffer, "0");
  }

  printf("Requested file offset: %s\n", buffer);

  fcntl(sd, F_SETOWN, getpid());
  /*
  // Setup SIGURG
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = signal_urgent;
  sigaction(SIGURG, &sa, NULL);
  */

  // Manually setting oobinline, though default is 0 already
  int oobinline = 0;
  setsockopt(sd,
    SOL_SOCKET,         /* Level */
    SO_OOBINLINE,       /* Option */
    &oobinline,         /* Ptr to value */
    sizeof (oobinline));

  if (connect(sd, (struct sockaddr *) &sin, sizeof(sin) ) < 0 ) {
    perror("\nОшибка при соединении с сервером: ");
    exit(EXIT_FAILURE);
  }

  if( send(sd, buffer, strlen(buffer), 0) < 0 ) {
    perror("\nОшибка при отправке смещения файла: ");
    exit(EXIT_FAILURE);
  }

  if((fd = fopen(filename, "a")) == NULL ) {
    perror("\nОшибка при открытии файла: ");
    exit(EXIT_FAILURE);
  }

  while (1) {
    /* if (sockatmark(sd)) */
    /* { */
    /*   size_t bytes_oob = recv(sd, &buffer_oob, 1, MSG_OOB); */
    /*   if (bytes_oob > 0) */
    /*   { */
    /*     printf("Received last/total: %zd/%zd\n", bytes_read, bytes_total); */
    /*   } */
    /* } */

    bytes_read = recv(sd, &buffer, 1, 0);
    if (bytes_read == -1)
    {
      perror("\nError reading data from socket: ");
      break;
    }
    if (bytes_read == 0)
    {
      printf("File receival complete.\n");
      break;
    }

    bytes_total += bytes_read;

    stat(filename, &st);
    fwrite(buffer, sizeof(buffer[0]), bytes_read, fd);
    fflush(fd);
  }
  fclose(fd);

  stat(filename, &st);
  printf("Total file size: %jd.\n", (intmax_t) st.st_size);
}

void udp_client(int sd) {
  int num;
  char buffer[BUF_SIZE];

  if((fd = fopen(filename, "a")) == NULL ) {
    perror("\nОшибка при открытии файла: ");
    exit(EXIT_FAILURE);
  }

  while(1) {

    // Open file and see if it exists
    if( access( filename, F_OK ) != -1 ) {
      stat(filename, &st);
      sprintf(buffer, "%lld", (long long) st.st_size);
    } else {
      strcpy(buffer, "0");
    }

    printf("Requested file offset: %s\n", buffer);

    rlen = sizeof(sin);

    num = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *) &sin, rlen);
    if(num < 0) {
      perror("\nError sending filesize to server: ");
      close(sd);
      exit(EXIT_FAILURE);
    }

    num = recvfrom(sd, buffer, sizeof(buffer), 0,(struct sockaddr *) &sin, &rlen);
    if(num < 0) {
      perror("\nError recieving from server: ");
      close(sd);
      exit(EXIT_FAILURE);
    }

    if (strcmp(buffer, end_message) == 0) {
      printf("\nEnd reading file\n");
      break;
    }

    fwrite(buffer, sizeof(buffer[0]), num, fd);
    fflush(fd);
  }

  fclose(fd);
}
