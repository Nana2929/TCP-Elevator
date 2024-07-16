#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "../config.hpp"
#include "elevator.hpp"

void handle_signal(int signal, int client_fd, Elevator &elevator) {
  char response[1024] = {0};
  switch (signal) {
  case '1':
    snprintf(response, sizeof(response), "Received signal 1");
    elevator.pressBtn(1);
    break;
  case '2':
    snprintf(response, sizeof(response), "Received signal 2");
    elevator.pressBtn(2);
    break;
  case '3':
    snprintf(response, sizeof(response), "Received signal 3");
    elevator.pressBtn(3);
    break;
  case '4':
    snprintf(response, sizeof(response), "Received signal 4");
    elevator.pressBtn(4);
    break;
  default:
    return; // Ignore all other signals
  }
  send(client_fd, response, strlen(response), 0);
}

void serve(Elevator &elevator) {

  int sock_fd, new_fd;
  socklen_t addrlen;
  struct sockaddr_in my_addr, client_addr;
  int status;
  char indata[1024] = {0};
  int on = 1;

  // create a socket
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    perror("Socket creation error");
    exit(1);
  }

  // for "Address already in use" error message
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
    perror("Setsockopt error");
    exit(1);
  }

  // server address
  my_addr.sin_family = AF_INET;
  inet_aton(HOST, &my_addr.sin_addr);
  my_addr.sin_port = htons(PORT);

  status = bind(sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr));
  if (status == -1) {
    perror("Binding error");
    exit(1);
  }
  printf("🛗 server starts at: %s:%d\n", inet_ntoa(my_addr.sin_addr), PORT);

  status = listen(sock_fd, 5);
  if (status == -1) {
    perror("Listening error");
    exit(1);
  }
  printf("wait for connection...\n");
  addrlen = sizeof(client_addr);
  // print this
  std::cout << "-----------------------------------------------" << std::endl;
  std::cout << "* ELEVATOR_SIGNAL_PRESS_1F     = 1" << std::endl;
  std::cout << "* ELEVATOR_SIGNAL_PRESS_2F     = 2" << std::endl;
  std::cout << "* ELEVATOR_SIGNAL_FLOOR_1_UP   = 3" << std::endl;
  std::cout << "* ELEVATOR_SIGNAL_FLOOR_2_DOWN = 4" << std::endl;
  std::cout << "-----------------------------------------------" << std::endl;

  while (true) {
    // connecting to client
    new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addrlen);
    printf("connected by %s:%d\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
    // serving one client
    while (true) {
      int nbytes = recv(new_fd, indata, sizeof(indata), 0);
      if (nbytes <= 0) {
        close(new_fd);
        printf("client closed connection.\n");
        break;
      }
      // Only handle the first character of indata
      if (nbytes > 0) {
        handle_signal(indata[0], new_fd, elevator);
      }
    }
  }
  close(sock_fd);
}

int main() {
  Elevator elevator;
  std::thread serve_thread(serve, std::ref(elevator));
  std::thread print_thread(&Elevator::printState, &elevator);
  std::thread control_thread(&Elevator::stateTransit, &elevator);
  print_thread.join();
  control_thread.join();
  serve_thread.join();
  return 0;
}