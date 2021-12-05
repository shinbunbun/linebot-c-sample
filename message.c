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

#define BUF_SIZE 1024

void reply(char *body)
{
  int mysocket;
  struct sockaddr_in server;
  struct addrinfo hints, *res;

  SSL *ssl;
  SSL_CTX *ctx;

  char msg[1000];

  // 環境変数からLINEのアクセストークンを取得
  char *token = getenv("TOKEN");

  // reply APIにわたすヘッダを作成
  char *header = (char *)malloc(sizeof(char) * BUF_SIZE);
  strcpy(header, "POST /v2/bot/message/reply HTTP/1.1\nHost: api.line.me\nContent-Type: application/json\nAuthorization: Bearer \0");
  /* printf("%s\n", header2); */
  strcat(header, token);
  strcat(header, "\0");
  /* printf("\n%s\n", header2); */

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
    return;
  }

  if ((mysocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
  {
    fprintf(stderr, "Fail to create a socket.\n");
    return;
  }

  if (connect(mysocket, res->ai_addr, res->ai_addrlen) != 0)
  {
    printf("Connection error.\n");
    return;
  }

  SSL_load_error_strings();
  SSL_library_init();

  ctx = SSL_CTX_new(SSLv23_client_method());
  ssl = SSL_new(ctx);
  err = SSL_set_fd(ssl, mysocket);
  SSL_connect(ssl);

  printf("Conntect to %s\n", host);

  strcat(header, "\nContent-Length: ");
  char l[1000];
  sprintf(l, "%ld", strlen(body));
  strcat(header, l);
  strcat(header, "\n");

  snprintf(msg, sizeof(msg), "%s\n%s", header, body);
  printf("%s\n", body);
  printf("%s\n", msg);
  SSL_write(ssl, msg, strlen(msg));

  int buf_size = 256;
  char buf[buf_size];
  int read_size;

  while (1)
  {
    /* SSLデータ受信 */
    int sslret = SSL_read(ssl, buf, buf_size);
    int ssl_eno = SSL_get_error(ssl, sslret);
    switch (ssl_eno)
    {
    case SSL_ERROR_NONE:
      break;
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_SYSCALL:
      continue;
    }
    break;
  }

  /* do
  {
    read_size = SSL_read(ssl, buf, buf_size); // ここに時間かかってる？
    write(1, buf, read_size);
  } while (read_size > 0); */

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

  // JSONファイルを読み込む
  root = json_loadb(buf, strlen(buf), 0, &error);
  // NULL=読込み失敗
  if (root == NULL)
  {
    printf("[ERR]json load FAILED\n");
    return;
  }

  // eventsの配列サイズ分ループを回す
  int events_size = json_array_size(json_object_get(root, "events"));
  for (int i = 0; i < events_size; i++)
  {
    // eventを取得
    json_t *event = json_array_get(json_object_get(root, "events"), i);

    // event typeを取得
    const char *type = json_string_value(json_object_get(event, "type"));
    // responseを作成
    struct event *response;

    // event typeがメッセージだった場合
    if (strcmp("message", type) == 0)
    {
      // messageオブジェクトを取得
      json_t *message = json_object_get(event, "message");
      // message typeを取得
      const char *message_type = json_string_value(json_object_get(message, "type"));
      // printf("%s\n", message_type);
      // message typeがtextだった場合
      if (strcmp("text", message_type) == 0)
      {
        // textを取得
        strcpy(text, json_string_value(json_object_get(message, "text")));
        // printf("%s\n", text);
        // reply_tokenを取得
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