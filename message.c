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

#include "verify.c"

#define HEADER_SIZE 8001
#define BODY_SIZE 22001
#define LARGE_BUF_SIZE (HEADER_SIZE + BODY_SIZE)

char *host = "api.line.me";
int port = 443;

void reply(char *body)
{
  int mysocket;
  // IPアドレス、ポート番号が入る構造体
  struct sockaddr_in server;
  // sockaddrをリンクリストで保持する構造体
  struct addrinfo hints, *res;

  // SSLconnectionごとに生成される構造体
  SSL *ssl;
  // SSLのコンテキスト構造体
  SSL_CTX *ctx;

  // 送信するリクエスト
  char msg[LARGE_BUF_SIZE];

  // 環境変数からLINEのアクセストークンを取得
  char *token = getenv("TOKEN");

  // reply APIにわたすヘッダを作成
  char *header = (char *)malloc(sizeof(char) * HEADER_SIZE);
  strcpy(header, "POST /v2/bot/message/reply HTTP/1.1\nHost: api.line.me\nContent-Type: application/json\nAuthorization: Bearer \0");
  /* printf("%s\n", header2); */
  strcat(header, token);
  strcat(header, "\0");
  /* printf("\n%s\n", header2); */

  char *path = "/v2/bot/message/reply";

  // IPアドレスの解決
  memset(&hints, 0, sizeof(hints));
  // IPv4
  hints.ai_family = AF_INET;
  // TCP
  hints.ai_socktype = SOCK_STREAM;
  // https
  char *service = "https";

  int err = 0;
  // IPアドレスの解決
  if ((err = getaddrinfo(host, service, &hints, &res)) != 0)
  {
    fprintf(stderr, "Fail to resolve ip address - %d\n", err);
    return;
  }

  // ソケットの作成
  if ((mysocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
  {
    fprintf(stderr, "Fail to create a socket.\n");
    return;
  }

  // TCPのconnectionを貼る
  if (connect(mysocket, res->ai_addr, res->ai_addrlen) != 0)
  {
    printf("Connection error.\n");
    return;
  }

  // libcryptoの全ての関数の全てのエラーメッセージをload
  SSL_load_error_strings();
  // 使用可能なcipherとdigestアルゴリズムを登録
  SSL_library_init();

  // SSL_CTXオブジェクトを生成
  // TLSはクライアントが対応している最高のバージョンが設定される
  ctx = SSL_CTX_new(SSLv23_client_method());
  // 証明書読み込み
  SSL_CTX_load_verify_locations(ctx, "line_server_cert/rootcacert_r3.pem", NULL);
  printf("【Certificate】\n\n");
  // サーバ証明書の検証
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);
  // SSL構造体を生成
  ssl = SSL_new(ctx);
  // ホスト名検証を行うようにする
  enable_hostname_validation(ssl, host);
  // For SNI
  SSL_set_tlsext_host_name(ssl, host);
  // SSL構造体にファイルディスクリプター（ソケット識別子）を設定
  err = SSL_set_fd(ssl, mysocket);
  // SSL/TLSハンドシェイクを開始
  SSL_connect(ssl);

  /* printf("Conntect to %s\n", host); */

  // headerにContent-Lengthを追加
  strcat(header, "\nContent-Length: ");
  char l[30];
  sprintf(l, "%ld", strlen(body));
  strcat(header, l);
  strcat(header, "\n");

  // 送信するリクエストを作成
  snprintf(msg, sizeof(msg), "%s\n%s", header, body);
  /* printf("%s\n", body);
  printf("%s\n", msg); */
  printf("%s\n", msg);
  // sslにバッファ（msg）を書き込む
  SSL_write(ssl, msg, strlen(msg));

  printf("\n\n");
  // データ受信
  while (1)
  {
    /* SSLデータ受信 */
    char *temp = (char *)malloc(sizeof(char) * BODY_SIZE);
    int sslret = SSL_read(ssl, temp, BODY_SIZE);
    printf("%s", temp);
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
  printf("\n");

  // TLS/SSLコネクションをシャットダウンする
  SSL_shutdown(ssl);
  // sslの参照カウントをデクリメント
  SSL_free(ssl);
  // ctxの参照カウントをデクリメント
  SSL_CTX_free(ctx);

  // ソケットをクローズ
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