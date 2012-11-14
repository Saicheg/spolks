#include "sockutils.h"
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char* argv[]) {
  if(argc < 4){
    perror("Params format: <HOST> <PORT> <FILENAME>");
    exit(EXIT_SUCCESS);
  }
  char *host = argv[1], *service = argv[2], *proto = "tcp";
  const char *filename = argv[3];
  struct sockaddr_in sin;
  int sd, n;
  FILE* fd;
  struct stat st;
  char offset[1024], buffer[1024];

  // Open file and see if it exists
  if( access( filename, F_OK ) != -1 ) {
    stat(filename, &st);
    sprintf(offset, "%lld", (long long) st.st_size);
  } else {
    strcpy(offset, "0");
  }

  printf("Requested file offset: %s\n", offset);

  if ((sd = mksock(host, service, proto,  &sin)) == -1) {
    perror("\nОшибка при создании сокета: ");
    exit(EXIT_FAILURE);
  }

  if (connect(sd, (struct sockaddr *) &sin, sizeof(sin) ) < 0 ) {
    perror("\nОшибка при соединении с сервером: ");
    exit(EXIT_FAILURE);
  }

  if( send(sd, offset, strlen(offset), 0) < 0 ) {
    perror("\nОшибка при отправке смещения файла: ");
    exit(EXIT_FAILURE);
  }


  if((fd = fopen(filename, "a")) == NULL ) {
    perror("\nОшибка при открытии файла");
    exit(EXIT_FAILURE);
  }
  while ( (n = read(sd, &buffer, sizeof(buffer))) > 0 ) {
    // Write to file
    stat(filename, &st);
    fwrite(buffer, sizeof(buffer[0]), n, fd);
    fflush(fd);
    printf("Written %d bytes at file offset %lld.\n",n, (long long) st.st_size);
  }
  fclose(fd);
  if ( n < 0 ) {
    perror("\nError receiving file:");
    exit(EXIT_FAILURE);
  }
  stat(filename, &st);
  printf("Total file size: %jd.\n",(intmax_t)st.st_size);

  close(sd);
  return EXIT_SUCCESS;
}
