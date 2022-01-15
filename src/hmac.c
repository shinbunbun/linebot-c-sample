/* https://kaworu.jpn.org/kaworu/2007-03-22-1.php */

#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <resolv.h>
#include <string.h>

int k64_encode(const char *hmac, int hmaclen, char *const encoded)
{

  BIO *mem = BIO_new(BIO_s_mem());
  printf("%s\n", hmac);
  // int buf_len = BIO_puts(mem, hmac);
  BIO *b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  // BIO_set_mem_eof_return(b64, BIO_NOCLOSE);
  BIO_push(b64, mem);
  BIO_write(b64, hmac, hmaclen);
  BIO_flush(b64);

  BIO_seek(mem, 0);
  // BIO_write(mem, "\n", 1);
  int buf_len = BIO_read(mem, encoded, 300);
  encoded[buf_len] = '\0';
  printf("%s\n", encoded);
  BIO_free_all(mem);

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
    if (k64_encode(res, reslen, buf) == -1)
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
