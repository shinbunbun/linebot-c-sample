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

#include "client.c"

#define HEADER_SIZE 8001
#define BODY_SIZE 22001
#define LARGE_BUF_SIZE (HEADER_SIZE + BODY_SIZE)

void create_message_obj(char *body, char *text, char *reply_token)
{
  // replyAPIに渡すbodyを作成
  strcpy(body, "{\"replyToken\":\"");
  strcat(body, reply_token);
  strcat(body, "\",\"messages\":[{\"type\":\"text\",\"text\":\"");
  strcat(body, text);
  strcat(body, "\"}]}");
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

void reply(char *buf)
{

  // replyAPIに渡すbody用のメモリを確保
  char *body = (char *)malloc(sizeof(char) * BODY_SIZE);

  // メッセージ、リプライトークンを格納するメモリを確保
  char *text = (char *)malloc(sizeof(char) * 10001);
  char *reply_token = (char *)malloc(sizeof(char) * 33);
  // JSONをパースしてtextとreply_tokenを取得
  parse(buf, text, reply_token);

  create_message_obj(body, text, reply_token);

  char *host = "api.line.me";
  // 環境変数からLINEのアクセストークンを取得
  char *token = getenv("TOKEN");

  // reply APIにわたすヘッダを作成
  char *header = (char *)malloc(sizeof(char) * HEADER_SIZE);
  strcpy(header, "POST /v2/bot/message/reply HTTP/1.1\nHost: api.line.me\nContent-Type: application/json\nAuthorization: Bearer \0");
  strcat(header, token);
  strcat(header, "\0");

  http_request req;
  req.header = header;
  req.body = body;
  req.host = host;

  request(req);

  free(reply_token);
  free(body);
  free(text);
  free(header);
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