symtest(SSL_write);
symtest(SSL_set_fd);
symtest(SSL_read);
symtest(SSL_get_error);
symtest(SSL_CTX_new);
symtest(SSL_new);
symtest(SSL_CTX_set_default_verify_paths);
symtest(SSL_connect);
symtest(SSL_get_peer_certificate);
symtest(SSL_get_verify_result);
symtest(SSL_free);
symtest(SSL_CTX_free);
symtest(SSL_set_verify);
symtest(SSL_set_verify_depth);
symtest(SSL_get_ex_data_X509_STORE_CTX_idx);
symtest(SSL_library_init);
symtest(SSL_load_error_strings);
symtest(SSL_CTX_get_cert_store);
symtest(SSL_CTX_set_verify);
symtest(SSL_get_current_cipher);
symtest(SSL_CIPHER_get_name);
symtest(SSL_CTX_load_verify_locations);

symtest(SSL_set_tlsext_host_name);
symtest(SSL_ctrl);

symtest(TLS_client_method);
symtest(SSLv23_client_method);
symtest(SSLv23_method);

symtest(SSL_shutdown);


