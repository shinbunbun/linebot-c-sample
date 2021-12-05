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

#define PORT 8765
#define BUF_SIZE 1024
#define LARGE_BUF_SIZE (int)1e5

int server()
{
  // SSLのコンテキスト構造体
  SSL_CTX *ctx;
  // SSLconnectionごとに生成される構造体
  SSL *ssl;

  // socketの識別子
  int server;

  // サーバ証明書と秘密鍵のパス
  char crt_file[] = "cert/fullchain1.pem";
  char key_file[] = "cert/key.pem";

  // IPアドレス、ポート番号が入る構造体
  struct sockaddr_in addr;
  socklen_t size = sizeof(struct sockaddr_in);

  /* char body[] = "hello world"; */
  /* printf("%s\n", header2); */

  // libcryptoの全ての関数の全てのエラーメッセージをload
  SSL_load_error_strings();
  // 使用可能なcipherとdigestアルゴリズムを登録
  SSL_library_init();
  // 全てのcipherとdigestアルゴリズムを内部テーブルに追加
  OpenSSL_add_all_algorithms();

  // SSL_CTXオブジェクトを生成
  // TLSはクライアントが対応している最高のバージョンが設定される
  ctx = SSL_CTX_new(TLS_server_method());
  // 証明書チェーンをロード
  SSL_CTX_use_certificate_chain_file(ctx, crt_file);
  // 秘密鍵をロード
  SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM);

  // ソケットを作成
  // IPv4, TCP
  server = socket(PF_INET, SOCK_STREAM, 0);

  // addrのメモリ領域を0バイトで消去する
  bzero(&addr, sizeof(addr));
  // IPv4
  addr.sin_family = AF_INET;

  // ローカルインターフェースを指定
  // 全てのローカルインターフェイスにバインドされうる
  addr.sin_addr.s_addr = INADDR_ANY;
  // ポート番号を指定
  addr.sin_port = htons(PORT);

  // ソケットとアドレス、ポートをバインド
  bind(server, (struct sockaddr *)&addr, sizeof(addr));
  // 接続の待受を開始
  listen(server, 10);

  // レスポンスヘッダを作成
  char header1[] = "HTTP/1.1 200 OK\nContent-Type: application/json";

  // 接続待機
  while (1)
  {

    printf("\n\n\n-------------waiting for client...-------------\n");

    // レスポンス用のメモリを確保
    char *msg = (char *)malloc(sizeof(char) * BUF_SIZE);
    // リクエストデータ用のメモリを確保
    char *buf = (char *)malloc(sizeof(char) * 1e5);
    /* char buf[(int)1e5]; */
    printf("-----------buf-----------\n%s\n-------------------------\n", buf);
    // ソケットの接続を待つ
    // addrにはクライアントのIPアドレスとポート番号が入る
    // clientにはクライアントのソケットの識別子が入る
    int client = accept(server, (struct sockaddr *)&addr, &size);

    printf(
        "Connection: %s:%d\n",
        inet_ntoa(addr.sin_addr),
        ntohs(addr.sin_port));

    // SSL構造体を生成
    // ctxの設定を継承する
    ssl = SSL_new(ctx);
    // SSL構造体にファイルディスクリプター（ソケット識別子）を設定
    SSL_set_fd(ssl, client);

    // クライアントがTLS/SSLハンドシェイクを開始するのを待つ
    // TODO: エラーハンドリング
    if (SSL_accept(ssl) > 0)
    {
      for (int i = 0;; i++)
      {
        // クライアントからのデータをいれるメモリを確保
        /* char temp[(int)1e5]; */
        char *temp = (char *)malloc(sizeof(char) * 1e5);
        /* printf(sizeof(temp)); */

        int r = SSL_read(ssl, temp, LARGE_BUF_SIZE);
        if (r == 0)
          break;
        if (i == 1)
        {
          // bodyを取得
          printf("%s\n", temp);
          strcpy(buf, temp);
        }
        free(temp);
      }
      printf("-----------buf-----------\n%s\n-------------------------\n", buf);

      // メッセージを格納するメモリを確保
      char *text = (char *)malloc(sizeof(char) * BUF_SIZE);
      // リプライトークンを格納するメモリを確保
      char *reply_token = (char *)malloc(sizeof(char) * BUF_SIZE);
      printf("text: %s\nreply_token: %s\n", text, reply_token);
      // JSONをパースしてtextとreply_tokenを取得
      parse(buf, text, reply_token);
      printf("text: %s, reply_token: %s\n", text, reply_token);

      // replyAPIに渡すbody用のメモリを確保
      char *body = (char *)malloc(sizeof(char) * BUF_SIZE);
      printf("%s\n", body);

      // replyAPIに渡すbodyを作成
      strcpy(body, "{\"replyToken\":\"");
      strcat(body, reply_token);
      strcat(body, "\",\"messages\":[{\"type\":\"text\",\"text\":\"");
      strcat(body, text);
      strcat(body, "\"}]}");

      printf("body: %s\n", body);

      // reply
      reply(body);

      int sres = snprintf(msg, sizeof(msg), "%s\n%s", header1, "Hello!");
      SSL_write(ssl, msg, strlen(msg));

      free(msg);
      free(buf);
      free(reply_token);
      free(body);
    }

    int sd = SSL_get_fd(ssl);
    SSL_free(ssl);
    close(sd);
  }

  close(server);
  SSL_CTX_free(ctx);

  return EXIT_SUCCESS;
}