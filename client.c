/*
 * Vitaliy Pavlenko
 * CS455
 * Project 1
 * Fall 2010
 *
 * Client issues commands to server for locking/unlocking ports and
 * queries server status
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <limits.h>

#include "common.h"
#include "protocol.h"

void print_help() {
  printf("Usage:\n"
	 " List of available commands and appropriate arguments\n"
	 "     lock <key> <IP Address> <port> <protocol>\n"
	 "  release <key> <IP Address> <port> <protocol>\n"
	 "   status <key> <IP Address> <port>\n"
	 "   status <key> <IP Address>\n"
	 "     exit <key> <IP Address>\n\n"
	 "Example:\n"
	 "  lock 111 127.0.0.1 5900 UDP\n"
	 "  exit 111 127.0.0.1\n");
}

void print_port(serv_response *res) {
  if (res->status == OK && res->port != 0)
    printf("%s %d %s %s\n", 
	   res->ip, 
	   res->port, 
	   PROTYPE2STR(res->protocol), 
	   S_PORTSTATE[res->state]);
  else
    printf("%s\n", S_REPLY[res->status]);
}

/*
 * Parse and print buffered response
 *
 * Because we're using re-entrant strtok_r, we have to preallocate char* pointers
 * for it to keep track of tokens and 20k port per process is a pretty high limit
 * on most OS's
 *
 * response msg consist of serv_response msgs, deliminated by ":" (DELIM)
 * see protocol.c for more details
 */
#define MAX_TOKENS 20000 
void print_response(char *msg) {
  char *tok, *sav[MAX_TOKENS];
  serv_response res;
  int count = 0;
 
  tok = strtok_r(msg, DELIM, sav);
  string2response(&res, tok);
  print_port(&res);

  while (NULL != (tok = strtok_r(NULL, DELIM, sav)) && count < MAX_TOKENS) {
    string2response(&res, tok);
    print_port(&res);
    count++;
  }
  
}

/*
 * Query server with a command
 * and print server reply to stdout
 */
unsigned int query_server(serv_command *com) {
  int serv_socket;
  struct sockaddr_in server_ip;
  char buff[MSG_SIZ] = {'\0'};
  char msg[MSG_SIZ*1000] = {'\0'};
  int byte_count;

  memset(&server_ip, 0, sizeof(server_ip));
  server_ip.sin_family = AF_INET;
  server_ip.sin_addr.s_addr = inet_addr(com->ip);
  server_ip.sin_port = htons((unsigned short)SERVER_PORT);

  if ((serv_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ||
      (connect(serv_socket, (struct sockaddr *) &server_ip, sizeof(server_ip)) < 0) ) {
    printf("%s\n",S_REPLY[NOSERVER]);
    return FALSE;
  }

  if (FALSE == command2string(buff, com))
    printf("%s\n", S_REPLY[ERROR]);

  if (send(serv_socket, buff, MSG_SIZ, 0) != MSG_SIZ) {
    printf("%s\n",S_REPLY[NOSERVER]);
    return FALSE;
  }
  
  /* Buffer up server's response */
  byte_count = 1;
  while (byte_count > 0) {
    byte_count = recv(serv_socket, buff, 1, 0);
    buff[1] = '\0';
    strncat(msg, buff, 1);
  }

  print_response(msg);
  return TRUE;
}

/*
 * Check command line argruments
 */
int parse_args(serv_command *com, int argc, char *argv[]) {
  char com_str[MSG_SIZ]  = { '\0' };
  int i = 1;
  while (i < argc) {
    strncat(com_str, argv[i], strlen(argv[i]));
    strncat(com_str, " ", 1);
    i++;
  }
  return string2command(com, com_str);
}

/*
 * Client main function.
 */
int main(int argc, char *argv[]) {
  serv_command com;
 
  if (FALSE == parse_args(&com, argc, argv)) {
    print_help();
    return 1;
  }

  if (TRUE == query_server(&com))
    return 0;
  return 1;
}
