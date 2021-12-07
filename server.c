/* https://ken-ohwada.hatenadiary.org/entry/2021/02/27/113436 */

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
#include "hmac.c"

#define PORT 8765

struct server_info
{
  // IPアドレス、ポート番号が入る構造体
  struct sockaddr_in addr;
  socklen_t size;
  // SSLのコンテキスト構造体
  SSL_CTX *ctx;
  int server;
};

int init_server(struct server_info *my_server_info)
{
  // サーバ証明書と秘密鍵のパス
  char crt_file[] = "cert/fullchain1.pem";
  char key_file[] = "cert/key.pem";

  my_server_info->size = sizeof(struct sockaddr_in);

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
  my_server_info->ctx = SSL_CTX_new(TLS_server_method());
  // 証明書チェーンをロード
  SSL_CTX_use_certificate_chain_file(my_server_info->ctx, crt_file);
  // 秘密鍵をロード
  SSL_CTX_use_PrivateKey_file(my_server_info->ctx, key_file, SSL_FILETYPE_PEM);

  // ソケットを作成
  // IPv4, TCP
  my_server_info->server = socket(PF_INET, SOCK_STREAM, 0);

  // addrのメモリ領域を0バイトで消去する
  bzero(&my_server_info->addr, sizeof(my_server_info->addr));
  // IPv4
  my_server_info->addr.sin_family = AF_INET;

  // ローカルインターフェースを指定
  // 全てのローカルインターフェイスにバインドされうる
  my_server_info->addr.sin_addr.s_addr = INADDR_ANY;
  // ポート番号を指定
  my_server_info->addr.sin_port = htons(PORT);

  // ソケットとアドレス、ポートをバインド
  bind(my_server_info->server, (struct sockaddr *)&my_server_info->addr, sizeof(my_server_info->addr));
  // 接続の待受を開始
  listen(my_server_info->server, 10);
}

void finish_server(struct server_info *my_server_info)
{
  // serverのソケットを閉じる
  close(my_server_info->server);
  // ctxを解放
  SSL_CTX_free(my_server_info->ctx);
}

void create_reply_body(char *body, char *text, char *reply_token)
{
  // replyAPIに渡すbodyを作成
  strcpy(body, "{\"replyToken\":\"");
  strcat(body, reply_token);
  strcat(body, "\",\"messages\":[{\"type\":\"text\",\"text\":\"");
  strcat(body, text);
  strcat(body, "\"}]}");

  /* printf("body: %s\n", body); */
}

void receive_SSL_data(SSL *ssl, char *header, char *body)
{
  printf("\n【Request】\n");
  for (int i = 0;; i++)
  {
    // クライアントからのデータをいれるメモリを確保
    /* char temp[(int)1e5]; */
    char *temp = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
    /* printf(sizeof(temp)); */

    /* SSLデータ受信 */
    int r = SSL_read(ssl, temp, LARGE_BUF_SIZE);
    printf("%s", temp);
    if (r == 0)
      break;
    if (i == 0)
    {
      strcpy(header, temp);
    }
    if (i == 1)
    {
      // bodyを取得
      strcpy(body, temp);
    }
    free(temp);
  }
  printf("\n");
}

void get_x_line_signature(char *header, char *x_line_signature)
{
  // headerからx-line-signatureを取得
  char *tmp = strstr(header, "x-line-signature");
  strncpy(x_line_signature, &tmp[18], 44);
  x_line_signature[44] = '\0';
}

void wait_connect(struct server_info *my_server_info)
{
  // 接続待機
  while (1)
  {

    printf("-------------waiting for client...-------------\n");

    // レスポンス用のメモリを確保
    char *msg = (char *)malloc(sizeof(char) * LARGE_BUF_SIZE);
    // リクエストデータ用のメモリを確保
    char *buf = (char *)malloc(sizeof(char) * BODY_SIZE);
    char *receive_header = (char *)malloc(sizeof(char) * HEADER_SIZE);
    // ソケットの接続を待つ
    // addrにはクライアントのIPアドレスとポート番号が入る
    // clientにはクライアントのソケットの識別子が入る
    int client = accept(my_server_info->server, (struct sockaddr *)&my_server_info->addr, &my_server_info->size);

    printf("Connection: %s:%d\n", inet_ntoa(my_server_info->addr.sin_addr), ntohs(my_server_info->addr.sin_port));

    // SSL connectionごとに生成される構造体
    // ctxの設定を継承する
    SSL *ssl = SSL_new(my_server_info->ctx);
    // SSL構造体にファイルディスクリプター（ソケット識別子）を設定
    SSL_set_fd(ssl, client);

    // クライアントがTLS/SSLハンドシェイクを開始するのを待つ
    // TODO: エラーハンドリング
    if (SSL_accept(ssl) > 0)
    {
      receive_SSL_data(ssl, receive_header, buf);

      // ヘッダから署名取り出し
      char x_line_signature[45];
      get_x_line_signature(receive_header, x_line_signature);

      // HMAC生成
      char sig[45];
      create_hmac(buf, sig, sizeof(sig));
      // HMAC検証
      if (strcmp(sig, x_line_signature) == 0)
      {
        printf("\nHMAC is valid\nHMAC: %s\n", sig);
      }
      else
      {
        fprintf(stderr, "\nHMAC is invalid\nx_line_signature: %s[end]\nsig: %s[end]\n", x_line_signature, sig);
        continue;
      }

      // replyAPIに渡すbody用のメモリを確保
      char *body = (char *)malloc(sizeof(char) * BODY_SIZE);

      // メッセージ、リプライトークンを格納するメモリを確保
      char *text = (char *)malloc(sizeof(char) * 10001);
      char *reply_token = (char *)malloc(sizeof(char) * 33);
      // JSONをパースしてtextとreply_tokenを取得
      parse(buf, text, reply_token);

      create_reply_body(body, text, reply_token);

      // reply
      reply(body);

      // レスポンスヘッダを作成
      char header1[] = "HTTP/1.1 200 OK\nContent-Type: application/json";

      // レスポンスを生成
      int sres = snprintf(msg, sizeof(msg), "%s\n%s", header1, "Hello!");
      // sslにバッファ（msg）を書き込む
      SSL_write(ssl, msg, strlen(msg));

      // 諸々解放
      free(msg);
      free(buf);
      free(reply_token);
      free(body);
    }

    // ソケット識別子を取得
    int sd = SSL_get_fd(ssl);
    // sslを解放
    SSL_free(ssl);
    // ソケットを閉じる
    close(sd);
  }
}

int server()
{

  struct server_info my_server_info;

  init_server(&my_server_info);

  wait_connect(&my_server_info);

  finish_server(&my_server_info);

  return EXIT_SUCCESS;
}