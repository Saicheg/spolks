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
char buffer[BUF_SIZE];
char buffer_oob[2];

int main(int argc, char* argv[]) {
  if(argc < 4){
    printf("Params format: <HOST> <PORT> <FILENAME>\n");
    exit(EXIT_SUCCESS);
  }

  char *host = argv[1], *service = argv[2], *proto = "tcp";
  const char *filename = argv[3];
  struct sockaddr_in sin;
  FILE* fd;
  struct stat st;

  // Open file and see if it exists
  if( access( filename, F_OK ) != -1 ) {
    stat(filename, &st);
    sprintf(buffer, "%lld", (long long) st.st_size);
  } else {
    strcpy(buffer, "0");
  }

  printf("Requested file offset: %s\n", buffer);

  if ((sd = mksock(host, service, proto,  &sin)) == -1) {
    perror("\nОшибка при создании сокета: ");
    exit(EXIT_FAILURE);
  }

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
    size_t bytes_oob = recv(sd, &buffer_oob, 1, MSG_OOB);
    if (bytes_oob > 0 && bytes_oob != -1)
    {
      printf("Received last/total: %zd/%zd\n", bytes_read, bytes_total);
    }

    bytes_read = recv(sd, &buffer, sizeof(buffer) * sizeof(buffer[0]), 0);
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
  printf("Total file size: %jd.\n",(intmax_t)st.st_size);

  close(sd);
  return EXIT_SUCCESS;
}
