#include <stdio.h>
#include <jansson.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define HEADER_SIZE 8001
#define BODY_SIZE 22001
#define LARGE_BUF_SIZE (HEADER_SIZE + BODY_SIZE)

typedef struct
{
  char *host;
  char *header;
  char *body;
} http_request;
