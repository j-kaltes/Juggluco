const char *(*ERR_reason_error_stringptr)(unsigned long e);
void (*ERR_error_string_nptr)(unsigned long e, char *buf, size_t len);
extern void (*ERR_print_errors_cbptr)(int (*cb)(const char *str, size_t len, void *u), void *u);
X509 *(*X509_STORE_CTX_get_current_certptr)(const X509_STORE_CTX *ctx);
int   (*X509_STORE_CTX_get_errorptr)(const X509_STORE_CTX *ctx);
int   (*X509_STORE_CTX_get_error_depthptr)(const X509_STORE_CTX *ctx);
char *(*X509_NAME_onelineptr)(const X509_NAME *a, char *buf, int size);
X509_NAME *(*X509_get_subject_nameptr)(const X509 *x);
char *(*X509_STORE_CTX_get_ex_dataptr)(X509_STORE_CTX *d, int idx);
const char *(*X509_verify_cert_error_stringptr)(long n);
void (*X509_freeptr)(X509 *a);
X509 *(*d2i_X509_fpptr)(FILE *fp, X509 **a);
int (*X509_STORE_add_certptr)(X509_STORE *ctx, X509 *x);
unsigned long (*ERR_get_errorptr)(void);








