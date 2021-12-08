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

typedef struct
{
  char *host;
  char *header;
  char *body;
} http_request;

void request(http_request req)
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
  if ((err = getaddrinfo(req.host, service, &hints, &res)) != 0)
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
  enable_hostname_validation(ssl, req.host);
  // For SNI
  SSL_set_tlsext_host_name(ssl, req.host);
  // SSL構造体にファイルディスクリプター（ソケット識別子）を設定
  err = SSL_set_fd(ssl, mysocket);
  // SSL/TLSハンドシェイクを開始
  SSL_connect(ssl);

  /* printf("Conntect to %s\n", host); */

  // headerにContent-Lengthを追加
  strcat(req.header, "\nContent-Length: ");
  char l[30];
  sprintf(l, "%ld", strlen(req.body));
  strcat(req.header, l);
  strcat(req.header, "\n");

  // 送信するリクエストを作成
  snprintf(msg, sizeof(msg), "%s\n%s", req.header, req.body);
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