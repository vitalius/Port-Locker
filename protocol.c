/*
 * Vitaliy Pavlenko
 * CS455
 * Project 1
 * Fall 2010
 *
 * This file contains functions for handling client requests
 */

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "common.h"
#include "protocol.h"

const char *S_REPLY[5] = {"QUIT", "AUTHFAIL", "NOSERVER", "OK", "ERROR"};
const char *S_COMMANDS[5] = {"LOCK", "RELEASE", "STATUS", "EXIT", "UNKNOWN"};
const char *S_PORTSTATE[4] = {"LOCKED", "FREE", "IN-USE", "PORT STATE UNKNOWN"};

/*
 * Parse a string into a serv_command struct.
 * Any command at least requires a command, ip and key.
 * Some commands require port and protocol type
 * Each command token is deliminated by space " "
 *
 *  Example:
 *
 *    str = "release 12345 127.0.0.1 UDP"
 *    str = "exit 12345 127.0.0.1"
 *
 * Returns 1(TRUE) on success and 0(FALSE) on failure.
 */
short string2command(serv_command *com, char *str) {
  char *tok;
  char *str_delim = " ";

  memset(com, 0, sizeof(com));
  memset(com->ip, 0, MAX_IP_LEN);
  uppercase(str);

  if (strlen(str) < 1) {
    com->command = UNKNOWN;
    return FALSE;
  }

  /* parse for command type */
  if (NULL == (tok = strtok(str, str_delim)))
    return FALSE;
  com->command = array_search(tok, S_COMMANDS, COMMANDS_LEN, UNKNOWN);

  /* secure key token */
  if (NULL == (tok = strtok(NULL, str_delim)))
    return FALSE;
  com->secure_key = strtol(tok, NULL, 10);

  /* ip */
  if (NULL == (tok = strtok(NULL, str_delim)))
    return FALSE;
  strncpy(com->ip, tok, NCPY(strlen(tok)));

  /* port */
  if (com->command == LOCK || com->command == RELEASE || com->command == STATUS) {
    if (NULL == (tok = strtok(NULL, str_delim))) {
      if (com->command == STATUS)
	com->port = 0;
      else
	return FALSE;
    } else {
      com->port = strtol(tok, NULL, 10);
    }
  }

  if (com->command == LOCK || com->command == RELEASE) {
    if (NULL == (tok = strtok(NULL, str_delim))) {
      return FALSE;
    } else {
      com->protocol = STR2PROTYPE(tok); 
    }
  }

  return TRUE;
}

short command2string(char *str, serv_command *com) {
  if (sprintf(str, "%s %ld %s %d %s", 
	      S_COMMANDS[com->command],
	      com->secure_key,
	      com->ip,
	      com->port,
	      PROTYPE2STR(com->protocol)) < 1)
    return FALSE;
  return TRUE;
}

/*
 * Used by client to convert incoming string message
 * into a server reply struct
 */
short string2response(serv_response *res, char *str) {
  char *tok;
  char *str_delim = " ";

  memset(res, 0, sizeof(res));
  memset(res->ip, 0, MAX_IP_LEN);
  uppercase(str);

  if (strlen(str) < 1)
    return FALSE;

  /* IP Address */
  if (NULL == (tok = strtok(str, str_delim)))
    return FALSE;
  strncpy(res->ip, tok, NCPY(strlen(tok)));

  /* Port number */
  if (NULL == (tok = strtok(NULL, str_delim))) {
    return FALSE;
  } else {
    res->port = strtol(tok, NULL, 10);
  }
  
  /* Protocol */
  if (NULL == (tok = strtok(NULL, str_delim))) {
    return FALSE;
  } else {
    res->protocol = STR2PROTYPE(tok); 
  }
  
  /* State of the port */
  if (NULL == (tok = strtok(NULL, str_delim)))
    return FALSE;
  res->state = array_search(tok, S_PORTSTATE, PORTSTATE_LEN, STATE_UNKNOWN);

  /* Message status */
  if (NULL == (tok = strtok(NULL, str_delim)))
    return FALSE;
  res->status = array_search(tok, S_REPLY, REPLY_LEN, UNKNOWN);

  return TRUE;
}

short response2string(char *str, serv_response *res) {
  if (sprintf(str, "%s %d %s %s %s%s", 
	      res->ip, 
	      res->port,
	      PROTYPE2STR(res->protocol),
	      S_PORTSTATE[res->state],
	      S_REPLY[res->status],
	      DELIM) < 1)
    return FALSE;
  return TRUE;
}


/*
 * Send entire list of all currently locked ports by pssvr.
 *
 */
void send_status(int sock, srv_conf *config) {
  int byte_count;
  char sending[MSG_SIZ] = { '\0' };
  serv_response res;

  memset(&res, 0, sizeof(res));
  memset(res.ip, 0, MAX_IP_LEN); 

  config->curr = config->head;
  while(config->curr) {
    strncpy(res.ip, config->curr->ip, NCPY(strlen(config->curr->ip)));
    res.port = config->curr->port;
    res.protocol = config->curr->protocol;
    res.status = OK;

    response2string(sending, &res);

    byte_count = send(sock, sending, MSG_SIZ, 0);
    if (byte_count < 0)
      printf("Error sending client a reply.\n");
    
    config->curr = config->curr->next;
  }
  config->curr = config->head;
}

/*
 * Send status of protocols for a given tupple (IP, port)
 */
void send_ip_port_status(int sock, srv_conf *config, char *ip, unsigned int port) {
  int byte_count;
  char sending[MSG_SIZ] = { '\0' };
  serv_response res;
  conf_list *curr;

  memset(&res, 0, sizeof(res));
  memset(res.ip, 0, MAX_IP_LEN);

  strncpy(res.ip, ip, NCPY(strlen(ip)));
  res.port = port;

  res.protocol = TCP;
  if (NULL == (curr = query_port(ip, port, TCP, config))) {
    if (TRUE == is_free(ip, port, TCP)) {
      res.state = FREE;
    } else {
      res.state = INUSE;
    }
  } else {
    res.state = LOCKED;
  }

  res.status = OK;
  response2string(sending, &res);  
  byte_count = send(sock, sending, MSG_SIZ, 0);
  if (byte_count < 0)
    printf("Error sending client a reply.\n");
    
  res.protocol = UDP;
  if (NULL == (curr = query_port(ip, port, UDP, config))) {
    if (TRUE == is_free(ip, port, UDP)) {
      res.state = FREE;
    } else {
      res.state = INUSE;
    }
  } else {
    res.state = LOCKED;
  }

  res.status = OK;
  response2string(sending, &res);  
  byte_count = send(sock, sending, MSG_SIZ, 0);
  if (byte_count < 0)
    printf("Error sending client a reply.\n");
}

/*
 * Returns 1 (TRUE) if EXIT command has been issued, 0 (FALSE) otherwise.
 *
 * Handles clients connection and commands.
 */
short handle_client(int client_sock, srv_conf *config) {
  int  byte_count;
  char recieved[MSG_SIZ] = { '\0' };
  char sending[MSG_SIZ] = { '\0' };
  serv_command com;
  serv_response res;

  memset(&res, 0, sizeof(res));
  memset(res.ip, 0, MAX_IP_LEN); 
  memset(&com, 0, sizeof(com));
  memset(com.ip, 0, MAX_IP_LEN);

  /* Read incoming command */
  byte_count = recv(client_sock, recieved, MSG_SIZ, 0);
  if (byte_count < 0)
    printf("Can't read from client.\n");

  if (FALSE != string2command(&com, recieved))
    if  (com.secure_key == config->secure_key) {

      /* do the command */
      switch (com.command) {
      case LOCK:
	if (TRUE == lock_port(com.ip, com.port, com.protocol, config)) {
	  print_locked(com.ip, com.port, com.protocol);
	  res.status = OK;
	} else
	  res.status = ERROR;
	break;
      case RELEASE:
	if (TRUE == release_port(com.ip, com.port, com.protocol, config)) {
	    print_released(com.ip, com.port, com.protocol);
	    res.status = OK;
	} else
	  res.status = ERROR;
	break;
      case STATUS:
	if (com.port != 0)
	  send_ip_port_status(client_sock, config, com.ip, com.port);
	else
	  send_status(client_sock, config);
	
	res.status = OK;
	break;
      case EXIT:
	res.status = QUIT;
	break;
      default:
	res.status = ERROR;
      }
    } else {
      print_auth_err(com.ip, com.port, com.secure_key);
      res.status = AUTHFAIL;
    }
  else
    res.status = ERROR;

  /* sending reply status */
  strncpy(res.ip, com.ip, NCPY(strlen(com.ip)));
  response2string(sending, &res);
  byte_count = send(client_sock, sending, MSG_SIZ, 0);
  if (byte_count < 0)
    printf("Error sending client a reply.\n");

  close(client_sock);

  return res.status;
}

