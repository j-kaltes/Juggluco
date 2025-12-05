#if defined(LOGALL)||defined(LOGLIBRE)
#define LIBRELOGGER(...)  LOGGER("VIEW: " __VA_ARGS__)
#define LIBRELOGAR(...) LOGAR("VIEW: " __VA_ARGS__)
#define LIBRELOGGERN(...) LOGGERN( __VA_ARGS__)
#define LIBRElogwriter(...); logwriter( __VA_ARGS__) 

#else
#define LIBRELOGGER(...)
#define LIBRELOGAR(...)
#define LIBRELOGGERN(...)
#define LIBRElogwriter(...)
#endif
