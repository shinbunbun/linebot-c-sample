#include <openssl/opensslv.h>

int main(void)
{
  printf("version: %s \n", OPENSSL_VERSION_TEXT);
}
