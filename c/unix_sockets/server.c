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
  strcpy(addr.sun_path, SERVER_SOCK);
  unlink(SERVER_SOCK);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  char msg_received[8192];
  struct sockaddr_un client_addr;
  socklen_t client_addr_len = sizeof(client_addr);

  printf("waiting to receive something...\n");

  char *msg_to_client = "programming \\o/";
  while ((recvfrom(fd, msg_received, 8192, 0, (struct sockaddr *)&client_addr,
                   &client_addr_len)) > 0) {
    printf("received question: %s\n", msg_received);

    int ret = sendto(fd, msg_to_client, strlen(msg_to_client) + 1, 0,
                     (struct sockaddr *)&client_addr, client_addr_len);
    if (ret < 0) {
      perror("sendto");
      break;
    }

    printf("answered: %s\n", msg_to_client);
  }

  if (fd >= 0) {
    close(fd);
  }

  return 0;
}
