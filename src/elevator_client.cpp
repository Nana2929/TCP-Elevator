#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.hpp"
// credit: https://shengyu7697.github.io/cpp-linux-tcp-socket/

int main() {
  int sock_fd;
  struct sockaddr_in serv_name;
  int status;
  char indata[1024] = {0}, outdata[1024] = {0};

  // create a socket
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    perror("Socket creation error");
    exit(1);
  }

  // server address
  serv_name.sin_family = AF_INET;
  inet_aton(HOST, &serv_name.sin_addr);
  serv_name.sin_port = htons(PORT);

  status = connect(sock_fd, (struct sockaddr *)&serv_name, sizeof(serv_name));
  if (status == -1) {
    perror("Connection error");
    exit(1);
  }

  while (1) {
    printf("please input message: ");
    fgets(outdata, sizeof(outdata), stdin);
    printf("send: %s\n", outdata);
    send(sock_fd, outdata, strlen(outdata), 0);

    int nbytes = recv(sock_fd, indata, sizeof(indata), 0);
    if (nbytes <= 0) {
      close(sock_fd);
      printf("server closed connection.\n");
      break;
    }
    printf("recv: %s\n", indata);
  }

  return 0;
}