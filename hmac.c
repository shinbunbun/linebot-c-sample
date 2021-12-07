/* https://kaworu.jpn.org/kaworu/2007-03-22-1.php */

#include <openssl/hmac.h>
#include <resolv.h>
#include <string.h>

int k64_encode(const char *data, size_t datalen, char *const buf, size_t len)
{
  int rv;

  rv = b64_ntop(data, datalen, buf, (len / sizeof(buf[0])));
  if (rv == -1)
  {
    fprintf(stderr, "b64_ntop: error encode base64");
    return (rv);
  }
  return (rv);
}

void create_hmac(char *data, char *buf, size_t buf_size)
{
  u_int reslen;
  char res[64 + 1];
  char *key = getenv("SECRET");
  size_t keylen = strlen(key);

  size_t datalen = strlen(data);

  if (HMAC(EVP_sha256(), key, keylen, data, datalen, res, &reslen))
  {
    if (k64_encode(res, reslen, buf, buf_size) == -1)
    {
      exit(EXIT_FAILURE);
    }
  }
}

#ifdef DEBUG
int main()
{
  char buf[1024];
  char data[1024] = "test";
  create_hmac(data, buf, sizeof(buf));
  printf("%s\n", buf);
  return 0;
}
#endif
