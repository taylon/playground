#include <unistd.h>
#include <stdio.h>
#include "stdlib.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#include "common.h"

int main() {
  int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, CLIENT_SOCK);
  unlink(CLIENT_SOCK);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, SERVER_SOCK);
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    perror("connect");

    exit(EXIT_FAILURE);
  }

  char message[8192];
  strcpy(message, "what is the most fun?");
  if (send(fd, message, strlen(message) + 1, 0) == -1) {
    perror("send");
    exit(EXIT_FAILURE);
  }

  printf("asked: %s\n", "what is the most fun?");

  if ((recv(fd, message, 8192, 0)) < 0) {
    perror("recv");
    exit(EXIT_FAILURE);
  }

  printf("answer received: %s\n", message);

  if (fd >= 0) {
    close(fd);
  }

  unlink(CLIENT_SOCK);

  return 0;
}
