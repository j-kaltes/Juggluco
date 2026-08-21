#ifndef L3_OPENSSL_FUNCTION_REDIRECTS_H
#define L3_OPENSSL_FUNCTION_REDIRECTS_H

/* Remap only libcrypto calls actually used by this project. */
#ifdef L3_USE_CRYPTO_DLSYM
#include "openssl_symbols.h"

#define EVP_DigestInit_ex EVP_DigestInit_exptr
#define EVP_DigestUpdate EVP_DigestUpdateptr
#define EVP_DigestFinal_ex EVP_DigestFinal_exptr
#define EVP_MD_CTX_new EVP_MD_CTX_newptr
#define EVP_MD_CTX_free EVP_MD_CTX_freeptr
#define EVP_sha1 EVP_sha1ptr
#define EVP_sha256 EVP_sha256ptr
#define EVP_sha512 EVP_sha512ptr
#define HMAC HMACptr
#define RAND_bytes RAND_bytesptr
#define EVP_CIPHER_CTX_new EVP_CIPHER_CTX_newptr
#define EVP_CIPHER_CTX_free EVP_CIPHER_CTX_freeptr
#define EVP_aes_256_ctr EVP_aes_256_ctrptr
#define EVP_EncryptInit_ex EVP_EncryptInit_exptr
#define EVP_EncryptUpdate EVP_EncryptUpdateptr
#define EVP_EncryptFinal_ex EVP_EncryptFinal_exptr
#endif

#endif /* L3_OPENSSL_FUNCTION_REDIRECTS_H */
