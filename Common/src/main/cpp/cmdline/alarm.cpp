#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

static void onalarm(int sig) {
	if(settings&&sensors) {
		LOGGER("onalarm do it\n");
		sensors->removeoldstates();
		}
	else
		LOGGER("onalarm no init\n");
	}

static void setalarm() {
	signal(SIGALRM,onalarm);
	time_t tim=time(nullptr);
	struct tm tmbuf;
	int op3am=3*60*60;
	int sincestart;
	if(localtime_r(&tim,&tmbuf)) {
		sincestart= tmbuf.tm_sec + (tmbuf.tm_min+ 60*tmbuf.tm_hour)*60;
		}
	else
		sincestart=0;
	if(sincestart>=op3am)
		op3am+=24*60*60;
	
	int waitoffset=op3am-sincestart;
	constexpr const int minwait=60*30;
	if(waitoffset<minwait)
		waitoffset=minwait;
        struct itimerval val { {60*60*24,0},{waitoffset,0}};
	LOGGER("set alarm %ld %ld\n",val.it_interval.tv_sec, val.it_value.tv_sec);
	setitimer(ITIMER_REAL,&val,nullptr);
	}
