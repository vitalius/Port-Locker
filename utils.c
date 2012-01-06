/*
 * Vitaliy Pavlenko
 * CS455
 * Project 1
 * Fall 2010
 *
 * This file contains various utility functions
 * for string matching, reading config files, binding sockets, etc.
 */

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "common.h"


/*
 * Parses one line from config file (after the secure key) into a linked-list element, port_line.
 *
 * Each configuration line is assumed to be in following format:
 *  IP_ADD PORT TYPE
 *
 * Example:
 *  255.255.255.255 1000 UDP
 *  10.0.0.1 52333 TCP
 *
 * In the event of any problem durring parsing with strtok()
 * this function prints an error message to stdout and returns FALSE.
 */
short parse_conf_line(char *ip, unsigned int *port, unsigned int *type, char *string) {
  char *tok;
  char *str_delim = " ";

  /* first token, IP address, string */
  tok = strtok(string, str_delim);
  if (tok == NULL) {
    printf("Error in parsing IP from a config file, line omited.\n");
    return FALSE;
  }
  strncpy(ip, tok, NCPY(strlen(tok)));

  /* second token, port */
  tok = strtok(NULL, str_delim);
  if (tok == NULL) {
    printf("Error in parsing PORT for %s, line omited.\n", ip);
    return FALSE;
  }
  *port = atoi(tok);


  /* third token, TCP or UDP */
  tok = strtok(NULL, str_delim);
  if (tok == NULL) {
    printf("Error in parsing PROTOCOL for %s:%d, line omited.\n", ip, *port);
    return FALSE;
  }
  *type = STR2PROTYPE(tok);

  return TRUE;
}


/*
 * Reads the configuration file
 */
srv_conf *load_config(const char *filename) {
  FILE *conf_file;
  char buffer[MSG_SIZ];
  srv_conf *config;

  config = (srv_conf *)malloc(sizeof(srv_conf));
  assert(config != NULL);

  conf_file = fopen(filename, "r");
  if (conf_file == NULL) {
    printf("Can't open config file %s\n", filename);
    exit(1);
  }
 
  /* Getting the authentication key, first line */
  if (NULL == fgets(buffer, MSG_SIZ, conf_file)) {
    printf("Failed to read %s\n", filename);
    exit(1);
  }
  config->secure_key = strtol(buffer, NULL, 10);


  /* all subsequent lines are assumed to be in IP_ADDR PORT TYPE format */
  /* omit all lines where parsing or locking function returns FALSE */
  while(fgets(buffer, MSG_SIZ, conf_file) != NULL) {
    char ip[MAX_IP_LEN] = { '\0' };
    unsigned int port, protocol;

    if (FALSE == parse_conf_line(ip, &port, &protocol, buffer))
      continue;

    if (FALSE == lock_port(ip, port, protocol, config))
      continue;
  }

  fclose(conf_file);
  return config;
}


/*
 * Prints current server config list
 */
void print_config(srv_conf *config) {
  config->curr = config->head;
  printf("Secure key: %ld\n", config->secure_key);
  while(config->curr) {
    printf("%s %d %s\n", 
	   config->curr->ip, 
	   config->curr->port,
	   PROTYPE2STR(config->curr->protocol));
    config->curr = config->curr->next;
  }
  config->curr = config->head;
}


/*
 * Attempt to bind a port specified by ip, port and type,
 * also create a locked list entry in srv_conf data structure
 *
 */
short lock_port(const char *ip, 
		const unsigned int port, 
		const unsigned short protocol, 
		srv_conf *config) {
  conf_list *item = NULL;
  int socket = 0;

  /* Attempt binding a socket to ip, port, protocol */
  if ((socket = bind_socket(ip, port, protocol)) < 0) {
    fprintf(stderr, "Error in binding a socket to %s:%d %s.\n", 
	    ip, port, PROTYPE2STR(protocol));
      return FALSE;
  }

  assert(NULL != (item = (conf_list *)malloc(sizeof(conf_list))));
  memset(item, 0, sizeof(item));
  memset(item->ip, 0, MAX_IP_LEN);

  /* Populating list item with info and adding to config list */
  strncpy(item->ip, ip, NCPY(strlen(ip)));
  item->port = port;
  item->protocol = protocol;
  item->socket = socket;

  /* Add at the begining of the list */
  item->next = config->head;
  config->head = item;

  return TRUE;
}


/*
 * Unlock port, also remove the entry from locked list.
 */
short release_port(const char *ip,
		   const unsigned int port,
		   const unsigned short protocol,
		   srv_conf *config) {

  conf_list *item = NULL;
  conf_list *prev = NULL;

  item = query_port(ip, port, protocol, config);
  if (NULL == item || config->head == NULL)
    return FALSE;
  
  if (FALSE == unbind_socket(item->socket))
    fprintf(stderr, "Unsuccesseful release of a socket for %s:%d %s.\n",
	    ip, port, PROTYPE2STR(protocol));
  

  prev = NULL;
  config->curr = config->head;
  while(config->curr) {
    if (0 == strcmp(ip, config->curr->ip)
	&& config->curr->port == port
	&& config->curr->protocol == protocol) {
      item = config->curr;
      break;
    }
    prev = config->curr;
    config->curr = config->curr->next;
  }
  config->curr = config->head;

  /* check if item in begining of the list */
  if (prev == NULL)
    config->head = item->next;
  else
    prev->next = item->next;

  free(item);

  return TRUE;
}


/*
 * Searches lock list for entry with a matching ip, port and type.
 * Returns pointer on success, NULL otherwise.
 */
conf_list *query_port(const char *ip,
		      const unsigned int port,
		      const unsigned short protocol,
		      srv_conf *config) {
  conf_list *result = NULL;

  config->curr = config->head;
  while(config->curr) {
    if (0 == strcmp(ip, config->curr->ip)
	&& config->curr->port == port
	&& config->curr->protocol == protocol) {
      result = config->curr;
      break;
    }
    config->curr = config->curr->next;
  }

  config->curr = config->head;
  return result;  
}


/*
 * Check if given [ip, port and protocol] is free on the system
 */
short is_free(const char *ip, 
	      const unsigned int port, 
	      const unsigned short protocol) {
  int s;

  s = bind_socket(ip, port, protocol);
  unbind_socket(s);

  if (s == -1)
    return FALSE; 

  return TRUE;
}


/*
 * Attempts to bind a scoket to an ip, port and type.
 *
 * Returns a socket descriptor
 */
short bind_socket(const char *ip, const unsigned int port, const unsigned short protocol) {
  struct sockaddr_in socket_ip;
  int socket_desc;

  memset(&socket_ip, 0, sizeof(socket_ip));

  socket_ip.sin_family = AF_INET;
  socket_ip.sin_addr.s_addr = inet_addr(ip);
  socket_ip.sin_port = htons(port);

  if (protocol == TCP) {
    if (-1 == (socket_desc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)))
      return -1;
  } else {
    if (-1 == (socket_desc = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)))
      return -1;
  }

  if (-1 == bind(socket_desc, (struct sockaddr *) &socket_ip, sizeof(socket_ip)))
    return -1;

  return socket_desc;
}


/*
 * Unbinds a socket descriptor, returns TRUE on success and FALSE otherwise.
 */
short unbind_socket(int socket_desc) {
  return (close(socket_desc) == 0 ? TRUE : FALSE);
}

/*
 * Uppercase entire string
 */ 
void uppercase(char *c) {
  while(*c != '\0') {
    *c = toupper((unsigned char)*c);
    c++;
  }
}

/*
 * Strips \n if one exists at the end of a string.
 */
short strip_nl(char *s) {
  int len = strlen(s);

  if (s[len-1] == '\n') {
    s[len-1] = '\0';
    return TRUE;
  }
  return FALSE;
}

/*
 * Checks if a string (*s) matches any strings in array, like in (see protocol.h)
 * If no matching command ids are found, returns default index.
 */
short array_search(char *s, const char **array, const int size, const int def) {
  int i;
  strip_nl(s);
  
  for(i = 0; i<size; i++)
    if(0 == strcmp(s, array[i]))
      return i;

  return def;
}

/*
 * Generate yyyy-mm-dd HH:MM:SS time stamp in a given string *s
 */
void gen_timestamp(char *s) {
  time_t lt;
  struct tm *now;
  
  lt = time(NULL);
  now = localtime(&lt);

  /* yyyy-mm-dd HH:MM:SS */
  sprintf(s,"%d-%d-%d %d:%d:%d", 
	  1900+now->tm_year, now->tm_mon, now->tm_mday,
	  now->tm_hour, now->tm_min, now->tm_sec);
}

/*
 * Prints authentication error log to stdout
 */
void print_auth_err(char *ip, unsigned int port, unsigned long key) {
  char time_str[MSG_SIZ];
  gen_timestamp(time_str);
  printf("%s INVALID CLIENT KEY(%ld) FROM %s", time_str, key, ip);
  if (port != 0)
    printf(" %d", port);
  printf("\n");
}

/* 
 * Logs locked port to stdout
 */
void print_locked(char *ip, unsigned int port, unsigned short protocol) {
  char time_str[MSG_SIZ];
  gen_timestamp(time_str);
  printf("%s %s %d %s LOCKED\n", time_str, ip, port, PROTYPE2STR(protocol));
}

/*
 * Logs released port to stdout
 */
void print_released(char *ip, unsigned int port, unsigned short protocol) {
  char time_str[MSG_SIZ];
  gen_timestamp(time_str);
  printf("%s %s %d %s RELEASED\n", time_str, ip, port, PROTYPE2STR(protocol));
}
