/* https://www.lisz-works.com/entry/jansson-for-wsl */
/* https://qiita.com/edo_m18/items/41770cba5c166f276a83 */

#include <stdio.h>
#include <jansson.h>
#include <string.h>

#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

void reply(char *header, char *body)
{
  int mysocket;
  struct sockaddr_in server;
  struct addrinfo hints, *res;

  SSL *ssl;
  SSL_CTX *ctx;

  char msg[100];

  char *host = "api.line.me";
  char *path = "/v2/bot/message/reply";
  int port = 443;

  // IPアドレスの解決
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  char *service = "https";

  int err = 0;
  if ((err = getaddrinfo(host, service, &hints, &res)) != 0)
  {
    fprintf(stderr, "Fail to resolve ip address - %d\n", err);
    return EXIT_FAILURE;
  }

  if ((mysocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
  {
    fprintf(stderr, "Fail to create a socket.\n");
    return EXIT_FAILURE;
  }

  if (connect(mysocket, res->ai_addr, res->ai_addrlen) != 0)
  {
    printf("Connection error.\n");
    return EXIT_FAILURE;
  }

  SSL_load_error_strings();
  SSL_library_init();

  ctx = SSL_CTX_new(SSLv23_client_method());
  ssl = SSL_new(ctx);
  err = SSL_set_fd(ssl, mysocket);
  SSL_connect(ssl);

  printf("Conntect to %s\n", host);

  snprintf(msg, sizeof(msg), "%s\r\n%s", header, body);
  SSL_write(ssl, msg, strlen(msg));

  int buf_size = 256;
  char buf[buf_size];
  int read_size;

  int response_len;

  while ((response_len = SSL_read(ssl, buf, 1024 - 1)) > 0)
  { /* SSL_readで読み込んだレスポンスをresponse_bufに格納.(1バイト以上なら続ける.) */

    /* response_bufの内容を出力. */
    printf("%s", buf);                   /* printfでresponse_bufを出力. */
    memset(buf, 0, sizeof(char) * 1024); /* memsetでresponse_bufをクリア. */
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  SSL_CTX_free(ctx);
  ERR_free_strings();

  close(mysocket);
}

void parse(char *buf, char *text, char *reply_token)
{
  // JSONオブジェクトを入れる変数
  json_error_t error;
  json_t *root;

  // JSONファイルをew読み込む
  root = json_loadb(buf, strlen(buf), 0, &error);
  // NULL=読込み失敗
  if (root == NULL)
  {
    printf("[ERR]json load FAILED\n");
    return 1;
  }

  int events_size = json_array_size(json_object_get(root, "events"));
  for (int i = 0; i < events_size; i++)
  {
    json_t *event = json_array_get(json_object_get(root, "events"), i);

    const char *type = json_string_value(json_object_get(event, "type"));
    struct event *response;
    if (strcmp("message", type) == 0)
    {
      json_t *message = json_object_get(event, "message");
      const char *message_type = json_string_value(json_object_get(message, "type"));
      // printf("%s\n", message_type);
      if (strcmp("text", message_type) == 0)
      {
        strcpy(text, json_string_value(json_object_get(message, "text")));
        // printf("%s\n", text);
        strcpy(reply_token, json_string_value(json_object_get(event, "replyToken")));
      }
    }
  }
  return;
}

#ifdef DEBUG
int main()
{
  char buf[] = "{\"destination\":\"Ucb7729a3788376675ee35bdb7228cbe5\",\"events\":[{\"type\":\"message\",\"message\":{\"type\":\"text\",\"id\":\"15189491325410\",\"text\":\"aaa\"},\"timestamp\":1638573412900,\"source\":{\"type\":\"user\",\"userId\":\"U6b53f4ad79a23f5427119cb44f08dbd7\"},\"replyToken\":\"10fe0b3180a84f35a1e1a5dabbd51447\",\"mode\":\"active\"}]}";
  char text[1000], reply_token[100];
  parse(buf, text, reply_token);
  printf("text: %s, reply_token: %s\n", text, reply_token);
}
#endif