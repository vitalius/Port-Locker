/*
 * Vitaliy Pavlenko
 * CS455
 * Project 1
 * Fall 2010
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

/* OSX port list command for debug
 * $ lsof -i 4 -a -p `lsof -i :15000 | grep "pssvr" | awk '{print $2}'` -P
 */

/* Definitions for extern const's are in protocol.c  */
/* Used for indexing reply strings, based on the command result */
#define REPLY_LEN 5
extern const char *S_REPLY[5];
enum {
  QUIT     = 0,
  AUTHFAIL = 1,
  NOSERVER = 2,
  OK       = 3,
  ERROR    = 4
};


/* Dictionary of protocol commands */
#define COMMANDS_LEN 5
extern const char *S_COMMANDS[5];
enum { 
  LOCK    = 0,
  RELEASE = 1, 
  STATUS  = 2, 
  EXIT    = 3,
  UNKNOWN = 4
};

/* Port status */
#define PORTSTATE_LEN 4
const char *S_PORTSTATE[4];
enum { 
  LOCKED        = 0,
  FREE          = 1, 
  INUSE         = 2,
  STATE_UNKNOWN = 3
};


typedef struct command {
  unsigned long secure_key; /* client authentication key */
  unsigned short command;   /* issued command, one of the *S_COM */
  char ip[MAX_IP_LEN];      /* ip, given as command argument */
  unsigned short port;      /* port and type arn't used by all commands */
  unsigned short protocol;    
} serv_command;

typedef struct response {
  char ip[MAX_IP_LEN];
  unsigned short port;
  unsigned short protocol;
  unsigned short state;
  unsigned short status;
} serv_response;

short handle_client(int client_sock, srv_conf *config);

short string2command(serv_command *com, char *str);
short command2string(char *str, serv_command *com);

short string2response(serv_response *res, char *str);
short response2string(char *str, serv_response *res);

void send_ip_port_status(int sock, srv_conf *config, char *ip, unsigned int port);
void send_status(int sock, srv_conf *config);
#endif
