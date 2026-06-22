#pragma once
struct passhost_t;
constexpr const int maxmirrortext=200;
extern char mirrorerrors[][maxmirrortext];
extern time_t mirrorerrorstimes[];

extern char *getmirrorerror(const passhost_t *pass) ;
extern char *getmirrorerrorsettime(const passhost_t *pass) ;
extern int savemessage(const passhost_t *pass,const char* fmt, ...);
#define saveerror(pass,...) savebuferror(getmirrorerror(pass),maxmirrortext,__VA_ARGS__)
extern void savebuferror(char *buf,int maxbuf,const char* fmt, ...);
