
int (*EVP_DigestInit_exptr)(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl)=nullptr;
int (*EVP_DigestFinal_exptr)(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s)=nullptr;
const EVP_MD *(*EVP_md5ptr)(void) = nullptr;
void (*AES_cfb128_encryptptr)(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char *ivec, int *num, const int enc) = nullptr;
