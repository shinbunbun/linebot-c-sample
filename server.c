#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

#include "message.c"

typedef struct
{
  char *body;
  char *header;
} httpResponse;

int server()
{
  SSL_CTX *ctx;
  SSL *ssl;

  int server, client, sd;
  int port = 8765;
  char crt_file[] = "cert/fullchain1.pem";
  char key_file[] = "cert/key.pem";

  struct sockaddr_in addr;
  socklen_t size = sizeof(struct sockaddr_in);

  char buf[(int)1e5];

  /* char body[] = "hello world"; */
  char header1[] = "HTTP/1.1 200 OK\r\nContent-Type: application/json";
  char header2[] = "GET /v2/bot/message/reply HTTP/1.1\r\nHost: api.line.me\r\nContent-Type: application/json\r\nAuthorization: Bearer ";
  char *token = getenv("TOKEN");
  strcat(header2, token);
  printf("%s\n", header2);
  char msg[1024];

  SSL_load_error_strings();
  SSL_library_init();
  OpenSSL_add_all_algorithms();

  ctx = SSL_CTX_new(SSLv23_server_method());
  SSL_CTX_use_certificate_chain_file(ctx, crt_file);
  SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM);

  server = socket(PF_INET, SOCK_STREAM, 0);
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  bind(server, (struct sockaddr *)&addr, sizeof(addr));
  listen(server, 10);

  while (1)
  {

    client = accept(server, (struct sockaddr *)&addr, &size);

    printf(
        "Connection: %s:%d\n",
        inet_ntoa(addr.sin_addr),
        ntohs(addr.sin_port));

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client);

    if (SSL_accept(ssl) > 0)
    {
      // rが0になるまで読み込まないとダメっぽい（なぜ？）
      for (int i = 0;; i++)
      {
        char temp[(int)1e5];
        int r = SSL_read(ssl, temp, sizeof(buf));
        if (r == 0)
          break;
        if (i == 1)
        {
          strcpy(buf, temp);
        }
      }
      printf("%s\n", buf);

      char text[1000], reply_token[100];
      parse(buf, text, reply_token);
      printf("text: %s, reply_token: %s\n", text, reply_token);

      char body[1000];

      strcpy(body, "{\"replyToken\":\"");
      strcat(body, reply_token);
      strcat(body, "\",\"messages\":[{\"type\":\"text\",\"text\":\"");
      strcat(body, text);
      strcat(body, "\"}]}");

      printf("body: %s\n", body);

      reply(header2, body);

      snprintf(msg, sizeof(msg), "%s\r\n%s", header1, "Hello!");
      SSL_write(ssl, msg, strlen(msg));
    }

    sd = SSL_get_fd(ssl);
    SSL_free(ssl);
    close(sd);
  }

  close(server);
  SSL_CTX_free(ctx);

  return EXIT_SUCCESS;
}