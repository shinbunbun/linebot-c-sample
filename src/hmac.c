/* https://kaworu.jpn.org/kaworu/2007-03-22-1.php */

#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <resolv.h>
#include <string.h>

int k64_encode(const char *data, char *const buf, size_t buf_size)
{

  BIO *mem = BIO_new(BIO_s_mem());
  BIO_puts(mem, data);
  BIO_read(mem, buf, buf_size);
  printf("%s\n", buf);

  /* BIO *bio, *b64;

  b64 = BIO_new(BIO_f_base64());
  FILE *output;
  bio = BIO_new_fp(stdout, BIO_NOCLOSE);
  BIO_push(b64, bio);
  BIO_write(b64, data, strlen(data));
  printf("0\n");
  BIO_read(bio, buf, buf_size);
  printf("1\n");
  printf("%s\n", buf);
  BIO_flush(b64);

  BIO_free_all(b64); */
}

void create_hmac(char *data, char *buf, size_t buf_size)
{
  unsigned int reslen;
  char res[64 + 1];
  char *key = getenv("SECRET");
  size_t keylen = strlen(key);

  size_t datalen = strlen(data);

  if (HMAC(EVP_sha256(), key, keylen, data, datalen, res, &reslen))
  {
    if (k64_encode(res, buf, buf_size) == -1)
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
