extern EVP_MD_CTX *(*EVP_MD_CTX_newptr)(void);
extern const EVP_MD *(*EVP_md5ptr)(void);
extern int (*EVP_DigestInit_exptr)(EVP_MD_CTX *ctx, const EVP_MD *type, ENGINE *impl);
extern int (*EVP_DigestUpdateptr)(EVP_MD_CTX *ctx, const void *d, size_t cnt);
extern int (*EVP_DigestFinal_exptr)(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s);
extern void (*EVP_MD_CTX_freeptr)(EVP_MD_CTX *ctx);
extern int (*AES_set_encrypt_keyptr)(const unsigned char *userKey, const int bits, AES_KEY *key);
extern void (*AES_cfb128_encryptptr)(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char *ivec, int *num, const int enc);
