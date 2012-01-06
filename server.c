/*
 * Vitaliy Pavlenko
 * CS455
 * Project 1
 * Fall 2010
 *
 * Main server function, starts and runs the listening loop
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "common.h"
#include "protocol.h"

int main(int argc, char *argv[]) {
  srv_conf *config;
  int server_sock;
  int client_sock;
  struct sockaddr_in server_ip;
  struct sockaddr_in client_ip;
  unsigned int client_ipl = sizeof(client_ip);

  /* Checking command option for config file */
  if (argc > 2 && !strncmp(argv[1], "-c", 2)) {
    config = load_config(argv[2]);
  } else {
    printf("Usage: %s <-c filname.config>\n", argv[0]);
    return 1;
  }

  /* setting up a listening socket */
  memset(&server_ip, 0, sizeof(server_ip));
  server_ip.sin_family = AF_INET;
  server_ip.sin_addr.s_addr = htonl(INADDR_ANY);
  server_ip.sin_port = htons((unsigned short)SERVER_PORT);

  /* binding a listening socket */
  if ((server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    printf("Can't bind to port %d.\n", SERVER_PORT);
    return 1;
  }

  if (bind(server_sock, (struct sockaddr *) &server_ip, sizeof(server_ip)) < 0) {
    printf("Can't bind to port %d.\n", SERVER_PORT);
    return 1;
  }
  
  if (listen(server_sock, 5) < 0) {
    printf("Can't bind to port %d.\n", SERVER_PORT);
    return 1;
  }

  /* process client requests */
  for (;;) {
    client_sock = accept(server_sock,
			(struct sockaddr *) &client_ip, 
			 &client_ipl);
    assert(client_sock > -1);

    /* If exit command has been properly issued, quit the loop */
    if (QUIT == handle_client(client_sock, config))
      break;
  }

  close(server_sock);

  return 0;
}

