/* https://www.bit-hive.com/articles/20200407 */
/* https://ken-ohwada.hatenadiary.org/entry/2021/02/27/113436 */

#include <openssl/ssl.h>

// X509_NAMEからコモンネームを取得する
char *get_x509_cmmon_name(X509_NAME *name)
{

  int idx = -1;

  unsigned char *utf8 = NULL;

  idx = X509_NAME_get_index_by_NID(name, NID_commonName, -1);

  X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, idx);

  ASN1_STRING *data = X509_NAME_ENTRY_get_data(entry);

  ASN1_STRING_to_UTF8(&utf8, data);

  return (char *)utf8;
}

// 証明書の内容を表示する
void print_info(X509_STORE_CTX *x509_ctx)
{
  // 証明書チェーンの深さを取得する
  int depth = X509_STORE_CTX_get_error_depth(x509_ctx);

  // 証明書を取得する
  X509 *cert = X509_STORE_CTX_get_current_cert(x509_ctx);

  // Subject名を取得する
  X509_NAME *sname = X509_get_subject_name(cert);
  char *subject = get_x509_cmmon_name(sname);

  // Issure名を取得する
  X509_NAME *iname = X509_get_issuer_name(cert);
  char *issuer = get_x509_cmmon_name(iname);

  printf("depth: %d \n", depth);
  printf("Subject: %s \n", subject);
  printf("Issuer: %s \n", issuer);
}

int verify_callback(int preverified, X509_STORE_CTX *ctx)
{
  // 証明書の内容を表示する
  print_info(ctx);

  if (preverified == 1)
  {
    printf("preverify OK \n");
  }
  else
  {
    // エラーコードを取得する
    int err = X509_STORE_CTX_get_error(ctx);

    if (err == X509_V_OK)
    {
      printf("verify OK \n");
    }
    else
    {
      printf("verify Error: %d \n", err);
    }
  }

  return preverified;
}

/* ホスト名検証 */
void enable_hostname_validation(SSL *ssl, const char *hostname)
{
  X509_VERIFY_PARAM *param;

  param = SSL_get0_param(ssl);

  X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
  if (!X509_VERIFY_PARAM_set1_host(param, hostname, 0))
  {
    ERR_print_errors_fp(stderr);
  }
}