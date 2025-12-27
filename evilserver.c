#include <ncurses.h>
#include <stdlib.h>
#include "conway.h"
#include "socket.h"

/**
 * A server which exploits a vulnerability in the messaging system,
 * allowing the server to accept a client, then immediately tell the client
 * it won the game.
 */
int main() {
  unsigned short port = 0;
  int server_socket = server_socket_open(&port);

  if (server_socket == -1) exit(-1);

  printf("Evil server listening on port %u\n",port);

  if (listen(server_socket, 1)) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }

  int client_socket = server_socket_accept(server_socket);
  if (client_socket == -1) {
    perror("accept failed");
  }

  printf("Found victim!\n");

  send_message(client_socket,"swin");

  printf("You won the set!\n");
  return 0;
}
