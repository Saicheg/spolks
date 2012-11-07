#include "sockutils.h"
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char* argv[]) {
  if(argc < 3){
    perror("wrong params format: <HOST> <PORT>");
    return -1;
  }
  char *host = argv[1], *service = argv[2], *proto = "tcp";
  const char *filename = "recieved";
  struct sockaddr_in sin;
  int sd, n, size;
  FILE* fd;
  struct stat st;
  char offset[1024], buffer[1024];
  ssize_t nrd;

  // Open file and see if it exists
  if( access( filename, F_OK ) != -1 ) {
    stat(filename, &st);
    sprintf(offset, "%lld", (long long) st.st_size);
  } else {
    strcpy(offset, "0");
  }

  printf("\nРазмер файла: %s\n", offset);

  if ((sd = mksock(host, service, proto,  &sin)) == -1) {
    perror("\nОшибка при подключении к сокету: ");
    exit(EXIT_FAILURE);
  }

  if (connect(sd, (struct sockaddr *) &sin, sizeof(sin) ) < 0 ) {
    perror("\nОшибка при соединении с сервером: ");
    exit(EXIT_FAILURE);
  }

  if( send(sd, offset, sizeof(offset), 0) < 0 ) {
    perror("\nОшибка при отправке размера файла: ");
    exit(EXIT_FAILURE);
  }



  while ( (n = read(sd, &buffer, sizeof(buffer))) > 0 ) {
    if( fd = fopen(filename, "w+") == NULL ) {
      perror("\nОшибка при открытии файла");
      exit(EXIT_FAILURE);
    }

    // Write to file
    fputs(buffer, stdout);
    fputs(buffer, fd);
    /* fwrite(buffer, sizeof(char), n, fd); */

    fclose(fd);
    /* write(fd, buffer, nrd); */
  }

  if ( n < 0 ) {
    printf("Ошибка при получении\n");
    exit(EXIT_FAILURE);
  }

  close(sd);
  return EXIT_SUCCESS;
}
