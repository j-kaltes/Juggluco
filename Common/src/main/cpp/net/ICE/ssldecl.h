int (*SSL_writeptr)(SSL *ssl, const void *buf, int num);
int (*SSL_set_fdptr)(SSL *ssl, int fd);
int (*SSL_readptr)(SSL *ssl, void *buf, int num);
int (*SSL_get_errorptr)(const SSL *ssl, int ret);
SSL_CTX *(*SSL_CTX_newptr)(const SSL_METHOD *method);
SSL *(*SSL_newptr)(SSL_CTX *ctx);
int (*SSL_CTX_set_default_verify_pathsptr)(SSL_CTX *ctx);
int (*SSL_connectptr)(SSL *ssl);
X509 *(*SSL_get_peer_certificateptr)(const SSL *ssl);
long (*SSL_get_verify_resultptr)(const SSL *ssl);
void (*SSL_freeptr)(SSL *ssl);
void (*SSL_CTX_freeptr)(SSL_CTX *ctx);
void (*SSL_set_verifyptr)(SSL *ssl, int mode, SSL_verify_cb verify_callback);
void (*SSL_set_verify_depthptr)(SSL *ssl, int depth);
int    (*SSL_get_ex_data_X509_STORE_CTX_idxptr)(void);
int (*SSL_library_initptr)(void);
void (*SSL_load_error_stringsptr)(void);
X509_STORE *(*SSL_CTX_get_cert_storeptr)(const SSL_CTX *ctx);
void (*SSL_CTX_set_verifyptr)(SSL_CTX *ctx, int mode, SSL_verify_cb verify_callback);
const SSL_CIPHER *(*SSL_get_current_cipherptr)(const SSL *ssl);
const char *(*SSL_CIPHER_get_nameptr)(const SSL_CIPHER *cipher);
int (*SSL_CTX_load_verify_locationsptr)(SSL_CTX *ctx, const char *CAfile, const char *CApath);

int (*SSL_set_tlsext_host_nameptr)(const SSL *s, const char *name);
long (*SSL_ctrlptr)(SSL *ssl, int cmd, long larg, void *parg);

const SSL_METHOD *(*TLS_client_methodptr)(void);
const SSL_METHOD *(*SSLv23_client_methodptr)(void);
const SSL_METHOD *(*SSLv23_methodptr)(void);

int (*SSL_shutdownptr)(SSL *ssl);


