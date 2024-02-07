#pragma once
struct passhost_t;
constexpr const int maxmirrortext=200;
extern char mirrorerrors[][maxmirrortext];

extern char *getmirrorerror(const passhost_t *pass) ;
extern int savemessage(const passhost_t *pass,const char* fmt, ...);
#define saveerror(pass,...) savebuferror(getmirrorerror(pass),maxmirrortext,__VA_ARGS__)
extern void savebuferror(char *buf,int maxbuf,const char* fmt, ...);
