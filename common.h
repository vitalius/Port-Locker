/*
 * Vitaliy Pavlenko
 * CS455
 * Project 1
 * Fall 2010
 */

#ifndef COMMON_H
#define COMMON_H

#define SERVER_PORT 10000

/* Size of a message sent via sockets */
#define MSG_SIZ 1024

/* Message Deliminator */
#define DELIM ":"

#define TRUE  1
#define FALSE 0

/* Any protocol specified at command line, that is not "tcp" is assumed as UDP. */
#define TCP  0
#define UDP  1

/* Macros for converting to and from TCP/UDP strings */
#define PROTYPE2STR(x) ((x == TCP) ? ("TCP") : ("UDP"))
#define STR2PROTYPE(x) ((x[0] == 'T' && x[1] == 'C' && x[2] == 'P') ? TCP : UDP)

#define MIN(x,y) ((x) < (y) ? (x) : (y))

/* 255.255.255.255 = 3 digits per quad + 3 dots for dlimeters + '\0' = 16 bytes */
#define MAX_IP_LEN  16

/* number of bytes to copy with strncpy, limitd by MAX_IP_LEN */
#define NCPY(x) (x < MAX_IP_LEN ? x : MAX_IP_LEN)


/*
 * A Linked-list to hold port tokens
 * all IP, PORT, TYPE tokens in this list are locked by the program.
 */
typedef struct list {
  char ip[MAX_IP_LEN];      /* char string for ip */
  unsigned short port;            
  unsigned short protocol;  /* TCP, UDP */
  int socket;               /* socket descriptor */
  struct list *next;
} conf_list;


/*
 * Server configuration data structure
 */
typedef struct conf {
  unsigned long secure_key;   /* client authentication key */
  conf_list *curr, *head;     /* linked-list pointers */
} srv_conf;


/*
 * Function headers 
 */

/* Config file parsing */
short parse_conf_line(char *ip, unsigned int *port, unsigned int *type, char *string);
srv_conf *load_config(const char *filename);

/* Debug stuff */
void print_config(srv_conf *config);

/* Strings stuff */
void uppercase(char *c);
short strip_nl(char *s);
short array_search(char *s, const char **array, const int size, const int def);

/* Socket stuff */
short bind_socket(const char *ip, const unsigned int port, const unsigned short type);
short unbind_socket(int socket_desc);

/* Linked list stuff */
short lock_port(const char *ip, const unsigned int port, const unsigned short type, srv_conf *config);
short release_port(const char *ip, const unsigned int port, const unsigned short type, srv_conf *config);
conf_list *query_port(const char *ip, const unsigned int port, const unsigned short type, srv_conf *config);
short is_free (const char *ip, const unsigned int port, const unsigned short type);

/* Log stuff */
void gen_timestamp(char *s);
void print_auth_err(char *ip, unsigned int port, unsigned long key);
void print_locked(char *ip, unsigned int port, unsigned short protocol);
void print_released(char *ip, unsigned int port, unsigned short protocol);
#endif
