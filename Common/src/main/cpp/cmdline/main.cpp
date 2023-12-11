/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 15:23:35 CET 2023                                                 */


#include <stdlib.h>

#include <signal.h>

#include "datbackup.h"
#include "nums/numdata.h"
#include "settings/settings.h"

//extern bool setfilesdir(const string_view filesdir,const char *country) ;
extern int startjuggluco(std::string_view dirfiles,const char *country) ;
extern char *localestr;
char localebuf[]="en_NL";

char *localestr=localebuf;
extern pathconcat numbasedir;
extern vector<Numdata*> numdatas;

extern bool networkpresent;

const char intro[]=R"(Command line program to create a Juggluco backup on a desktop computer.
Juggluco is an android applet that connects with Freestyle libre 2 sensors
and allows one to add diabetes diary data, see:
https://www.juggluco.nl/Juggluco
Within a certain directory (-d dir) backup and connection data is saved.
With this program, you can specify the connections the program
should receive data from and send data to.
)";
      #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

template<typename P,typename  ... Ts>
bool  exportdata(const char *filename,P proc,Ts... args)  {
	int han=creat(filename, S_IRUSR| S_IWUSR);
	if(han<0) {
		cerr<<"Can't open "<<filename<<endl;
		return false;
		}
	if(!proc(han,args ...)) {
		cerr<<"Export failed"<<endl;
		return false;
		}
	
	cout<<"Exported to "<<filename<<endl;
	return true;
	}


extern bool allsavemeals(int handle,uint32_t starttime=0,uint32_t endtime=UINT32_MAX);


extern bool exportnums(int handle,uint32_t starttime=0,uint32_t endtime=UINT32_MAX);
bool exportnummer(const char *filename,uint32_t starttime=0,uint32_t endtime=UINT32_MAX) {
	return exportdata(filename,exportnums,starttime,endtime);
	}
/*
bool exportnums(const char *filename) {
	int han=creat(filename, S_IRUSR| S_IWUSR);
	if(han<0) {
		cerr<<"Can't open "<<filename<<endl;
		return false;
		}
	if(!exportnums(han)) {
		cerr<<"Export failed"<<endl;
		return false;
		}
	
	cerr<<"Amounts exported to "<<filename<<endl;
	return true;
	}
bool exporthistory(int handle) {


	}
		case 3: return exporthistory(fd);
bool exportnums(int handle) {

		case 0: return exportnums(fd);	;
		case 1: return exportscans(fd, &SensorGlucoseData::getScandata);
		case 2: return exportscans(fd, &SensorGlucoseData::getPolldata);
		case 3: return exporthistory(fd);
*/

void help(const char *progname) {
auto aip=myip();
const char *hostip=aip.data();
cout<<intro<<"Usages: "<<progname<<R"( -d dir : save data in directory dir)"<<endl
 <<"        "<<progname<<R"( [-d dir] -p port : listen on port port)"<<endl
 <<"        "<<progname<<R"( [-d dir] -l : list configuration data)"<<endl
 <<"        "<<progname<<R"( [-d dir] -c : clear configuration data)"<<endl
 <<"        "<<progname<<R"( -x[-+]: start eXport/xDrip/Nightscout web server (https://www.juggluco.nl/Juggluco/webserver.html) or not (-x-))"<<endl
 <<"        "<<progname<<R"( -X[-+]:  the same but server can also be assessed remotely)"<<endl
 <<"        "<<progname<<R"( -g secret: use as api_secret secret
)"
#ifdef USE_SSL
R"(        )"<<progname<<R"( -e[-+]:  Use web server with SSL encryption)"<<endl
 <<"        "<<progname<<R"( -o port:  use port for the SSL server)"
#else
R"(Compile a dynamically linked version of )"<<progname<<R"( for SSL)"
#endif
R"(
	)"<<progname<<R"( -[0123456789][RLCOD] place label 0-9 in category R,L,C,O or D:
	R: Rapid acting Insulin
	L: Long acting Insulin
	C: Carbohydate
	O: cOmments
	D: Don't send
	-0R means label 0 (first one) is Rapid acting insulin
	-1C means label 1 (second) is Carbohydrate
	-2O means give label 2 as a cOmment
        )"<<progname<<R"( [-d dir] -t[+]: give treatments/amounts via Nightscout interface
        )"<<progname<<R"( [-d dir] -N filename: export nums 
	)"<<progname<<R"( [-d dir] -S filename: export scans 
	)"<<progname<<R"( [-d dir] -B filename: export stream 
	)" << progname<<R"( [-d dir] -H filename: export history 
	)"<<progname<<R"( [-d dir] -m filename: export meals 
	)" << progname<<R"( [-d dir] -M : mmol/L
	)" << progname<<R"( [-d dir] -G : mg/dL
	)" << progname<<R"( [-d dir] -R n : remove n-th connection
	)" << progname<<R"( [-d dir] -Z n : resend data to n-th connection
	)" << progname<<R"( -v  : version

	)"<< progname<<R"( [-d dir] OPTIONS IP1,IP2 ...  : Specify connection with IP(s).
OPTIONS:
	-r: receive from host
	-a: automatically detect IP the first time
	-A: Active only. Don't accept connections from this host. Only actively connect to this host.
	-P: Passive only. Never connect actively to this host. Only accepts connections.

	-n: send nums (amounts)
	-s: send scans (via NFC received data)
	-b: send stream (via bluetooth received data)

	-L label: give connection label 'label'. A connection is only established if both side specify 
		the same label.
	-i: don't test on ip
	-w password: encrypt communication with password
	-p port:  connect to port (default: 8795)

	-C n: overwrite n-th connection. If this program sends to this connection, data will not be resend. 

example:

)"<<progname<<R"( -d mydir -r  192.168.1.64

Receive data from host 192.168.1.64 (an ip shown in backup screen android app)
In the juggluco android applet you can in "backup, add connection"
specify send to amounts, scans and stream and specify port 8795 and
an ip of this computer (e.g )"<<hostip<<")\n\n"<<progname<<R"( -d mydir -nsb  192.168.1.69

Send the amounts, scans and stream data to 192.168.1.69 on the default port (8795)
In the android applet you can in the "add connection" dialog, specify 
an ip of this computer ()"<<hostip<<R"() check "Receive from" and specify as port 8795
(the port specified earlier for this computer)

)"<<progname<<R"( -d mydir

Starts the program with this configuration.
)";

	};
int showui;
bool getpassive(int pos);
bool getactive(int pos); 
static std::string_view treatmenttype(int labelnr) {
	if(labelnr>=settings->data()->varcount||labelnr<0) {
		return "No label index";
		}
	auto *nums=settings->data()->Nightnums;
	const int index=nums[labelnr].kind;
	static const std::string_view typenames[]={"Not set","Rapid acting insulin","Long acting insulin","Carbohydrate","Comments","Don't send"};
	if(index>=sizeof(typenames)||index<0) {
		return "Out of Range";
		}
	return typenames[index];
	}
int   listconnections() {
	if(settings->data()->usexdripwebserver) {
		cout<<"eXport web server turned on (port 17580)\n";
		if(settings->data()->remotelyxdripserver) {
			cout<<"can also be used remotely over http\n";
			}
		else
			cout<<"http only over localhost\n";

#ifdef USE_SSL
		if(settings->data()->useSSL) 
			cout<<"Use SSL, sslport="<<	settings->data()->sslport<<endl;
		else
			cout<<"Do not use SSL"<<endl;
#else
			cout<<"Compile a dynamically linked version of juggluco server for SSL"<<endl;
#endif
		Tings::Variables *varsptr=settings->data()->vars;
		const int labnr=settings->data()->varcount;
		for(int i=0;i<labnr;i++) {
			cout<<left<<i<<": "<<setw(12)<<varsptr[i].name<<treatmenttype(i)<<endl;
			}
		cout<<"api/v1/treatments turned "<<(settings->data()->saytreatments?"on":"off")<<endl;
		if(settings->data()->apisecretlength) {
			settings->data()->apisecret[settings->data()->apisecretlength]='\0';
			cout<<"api_secret: "<<settings->data()->apisecret<<endl<<endl;
		}else  {
			cout<<"No api_secret\n\n";
			}
		}
	else {
		cout<<"eXport web server turned off\n\n";
		}
	const int hostnr=backup->gethostnr();
	cout<<"Mirror port "<< backup->getupdatedata()->port <<endl;
	cout<<"unit: "<<settings->getunitlabel()<<endl;
	cout<<"connection"<<(hostnr>1?"s:":":")<<endl;;
	for(int h=0;h<hostnr;h++) {
		cout<<h+1<<": ";
		const passhost_t &host=backup->getupdatedata()->allhosts[h];
		cout<< (host.hasname?host.getname():"")<<(host.detect?" detect,":" ")<< (host.receivefrom&2?" receiver":"")<<(getpassive(h)?" passiveonly ":" ")<<(getactive(h)?" active only ":"")<<(host.haspass()?backup->getpass(h).data():"no pass") <<", port="<< ntohs(host.ips[0].sin6_port);
		int sin=host.index;
		if(sin>=0) {
			const updateone &sto=backup->getupdatedata()->tosend[sin];
			const bool sen=sto.sendnums || sto.sendstream|| sto.sendscans;
			if(sen) 
				cout<<" send " <<(sto.sendnums?"nums ":"")<<(sto.sendscans?"scans ":"")<<(sto.sendstream?"stream ":"");
			}
		const int len=host.nr;
		for(int i=0;i<len;i++) {
			namehost name(host.ips+i);
			cout<<" "<<name.data();
			}
		cout<<endl;
		}
	return 1234;
	}
int clearhosts() {
		
	backup->resetall();	
	return 1234;
	}
//#if defined(_Windows) || __ANDROID__
const char dirconf[]=".jugglucorc";

//bool exportscans(int handle, const std::span<const ScanData>  (SensorGlucoseData::*proc)(void) const) ;

//bool exportscans(int handle, const std::span<const ScanData>  (SensorGlucoseData::*proc)(void) const) ;

template <bool repeatids>
bool exportscans(int handle,  CurData  (SensorGlucoseData::*proc)(const uint32_t,const uint32_t) const,uint32_t starttime=0,uint32_t endtime=UINT32_MAX) ;
bool exporthistory(int handle,uint32_t starttime=0,uint32_t endtime=UINT32_MAX) ;
void showversion() {
#include "version.h"
	cout<<"Version "<< APPVERSION <<endl;
	}

template <int N> bool setlabeltype(const int (&types)[N]) {
	auto *nums=settings->data()->Nightnums;
	bool did=false;
	for(int i=0;i<N;i++) {
		bool didnow=true;
		switch(toupper(types[i])) {
			case 'R': nums[i].kind=1;nums[i].weight=1.0f;break;
			case 'L':nums[i].kind=2;nums[i].weight=1.0f;break;
			case 'C':nums[i].kind=3;nums[i].weight=1.0f;break;
			case 'O': nums[i].kind=4;break;
			case 'D': nums[i].kind=5;break;
			default: didnow=false; 
			}
		did =did||didnow;
		}
	return did;
	}

 Readall alldir;

int readconfig(int argc, char **argv) {
	bool receive=false,detect=false,signal=false,nums=false,scans=false,stream=false;
	bool list=false,clear=false;
	const char *password="";
	char *dir=nullptr;
	string_view port;

	int rmindex=-1,reinitpos=-1;;
const char * numexport=nullptr;
const char *historyexport=nullptr;
const char *scanexport=nullptr;
const char *pollexport=nullptr;
const char *mealexport=nullptr;
const char *label=nullptr;
bool xremote=false,activeonly=false,passiveonly=false,testip=true

#ifndef DONT_USE_LIBREVIEW
,showlibreview=false
#endif
;
int xdripserver=-1,use_ssl=-1,give_treatments=-1;
int changer=-1;
int unit=0;

int labeltype[maxvarnr]{};
uint32_t starttime=0,endtime=UINT32_MAX;
char *api_secret=nullptr,*sslport=nullptr;
           for(int opt;(opt = getopt(argc, argv, "Z:o:e::g:p:d:lcX::x::ransvzibAPw:hN:S:B:H:m:GMR:L:C:0:1:2:3:4:5:6:7:8:9:t::")) != -1;) {
	   	if(opt>='0'&&opt<='9') {
			int num=opt-'0';
			if(optarg[0]>='0'&&optarg[0]<='9') {
				num=num*10+optarg++[0]-'0';
				if(num>=maxvarnr) {
					cerr<<num<<" is too large, maximal "<<maxvarnr<<" labels allowed\n";
					return 10;
					}
				}
			labeltype[num]=optarg[0];
			cout<<num<<": "<<optarg[0]<<endl;
			}
		else {
               switch (opt) {
	       	   case 'v': showversion();return 1234;
	           case 'i': testip=false; break;
		   case 'X': xremote=true;
		   	case 'x': {
				if(optarg) {
					cerr<<"eXport arg:"<<optarg<<':'<<endl;
					if(!optarg[1]) {
						switch(optarg[0]) {
							case '+': xdripserver=1;goto XDRIPSUCCESS;break;
							case '-': xdripserver=0;goto XDRIPSUCCESS;break;
							};
						};
					cerr<<"Unknown arg: "<<optarg<<endl;;
					cerr<<"Argument to -x should be + or -\n";
					return 10;
					}
				else {
					cerr<<"eXport arg\n";
					xdripserver=1;
					}

				XDRIPSUCCESS:
				break;
				}
			case 'd':
				dir=optarg;
				break;
		       case 'r':
			   receive=true;
			   break;
		       case 'a':
			   detect=true;
			   break;
		       case 'G': unit=2;break;
		       case 'Z':
				reinitpos=atoi(optarg); 
				cerr<<"Reinit "<<reinitpos--<<endl;
				break;
		       case 'R': rmindex=atoi(optarg); 
			cerr<<"remove "<<rmindex--<<endl;
		       		break;
		       case 'C': changer=atoi(optarg)-1; 
		       		break;
			case 'L': label=optarg;break;
		       case 'M': unit=1;break;
		       case 'P': passiveonly=true;break;
		       case 'A': activeonly=true;break;
		       case 'n': nums=true;break;
		       case 's': scans=true;break;
		       case 'b': stream=true;break;
		       case 'w': password=optarg;break;
		       case 'p': port=optarg;
		       if(port.size()>5) {
		       		cerr<<"%s too large, port is maximally 5 digits\n";
				return 7;
		       		}
		       	
		       	break;
		       case 'l': list=true;break;;
		       case 'c': clear=true;break;
		       case 'N': numexport=optarg;break;
		       case 'S': scanexport=optarg;break;
		       case 'B': pollexport=optarg;break;
		       case 'H': historyexport=optarg;break;
		       case 'm': mealexport=optarg;break;
#ifndef DONT_USE_LIBREVIEW
		       case 'z': showlibreview=true;break;
#endif
			case 'g': api_secret=optarg;break;  //api_secret;
			case 'o': sslport=optarg;break;  ///sslport
			case 'e': 
				if(optarg) {
					#define toshort(x) x[0]|(x[1]<<8)

					cerr<<"use_ssl:"<<optarg<<':'<<endl;
					const short arg=toshort( optarg);
					switch(arg) {
						case toshort("+"): use_ssl=1;;break;
						case toshort("-"): use_ssl=0;;break;
						default:
							cerr<<"Unknown arg: "<<optarg<<endl;;
							cerr<<"Argument to -e should be + or -\n";
							return 10;
						};
					}
				else {
					cerr<<"use_ssl\n";
					use_ssl=1;
					}

				break;
			case 't': 
				if(optarg) {
					#define toshort(x) x[0]|(x[1]<<8)

					cerr<<"treatments:"<<optarg<<':'<<endl;
					const short arg=toshort( optarg);
					switch(arg) {
						case toshort("+"): give_treatments=1;;break;
						case toshort("-"): give_treatments=0;;break;
						default:
							cerr<<"Unknown arg: "<<optarg<<endl;;
							cerr<<"Argument to -t should be + or -\n";
							return 10;
						};
					}
				else {
					cerr<<"treatments\n";
					give_treatments=1;
					}

				break;



		       default: help(argv[0]); return 4;
		       }
		       }
           }
static constexpr const	char defaultname[]="jugglucodata";
	std::string_view uitdir;
	if(!dir) {
		if(!alldir.fromfile(dirconf)) {
			uitdir={defaultname,sizeof(defaultname)-1};
			}
		else {
			uitdir=alldir;
			}
		}
	else {
		int dirlen=strlen(dir)-1;
		for(;dir[dirlen]=='/'||dir[dirlen]=='\\';dirlen--)
			dir[dirlen]='\0';	
		if(!writeall(dirconf,dir,dirlen+1)) {
			cerr<<"Write to "<<dirconf<<" failed\n";
			}
		alldir.assign(dir,dirlen);
		uitdir=alldir;
		}
	cout<<"Saving in directory "<<uitdir.data()<<endl;
	bool did=false;
	switch(startjuggluco(uitdir,nullptr)) {
		case 0:break;
		default: return 10;
		};
	did=did||setlabeltype(labeltype);

	if(sslport) {
		int port;
		if(sscanf(sslport,"%d",&port)<=0) {
			perror("sscanf");
			return 10;
			}
		if(port<1024||port>65535) {
			cerr<<"port should be between 1024 and 65535\n";
			return 10;
			}
		settings->data()->sslport=port;
		did=true;
		}
	if(api_secret) {
		int len=strlen(api_secret);
		if(len>80) {
			cerr<<"Maximal api_secret is 80 bytes\n";
			return 10;
			}
		settings->data()->apisecretlength=len;
		memcpy(settings->data()->apisecret,api_secret,len+1);
	extern void	makesha1secret();
		makesha1secret();
		did=true;
		}
	if(use_ssl>=0) {
		settings->data()->useSSL=use_ssl;
		did=true;
		}
	if(give_treatments>=0) {
		settings->data()->saytreatments=give_treatments;
		did=true;
		}
	if(xdripserver>=0) {
		settings->data()->usexdripwebserver=xdripserver;
		settings->data()->remotelyxdripserver=xremote;
		did=true;
		}
	if(unit)  {
		settings->setunit(unit);
		did=true;
		}
	else
		settings->setlinuxcountry();
//	constexpr size_t nummmaplen=77056;
	 if(Numdata* numdata=Numdata::getnumdata( pathconcat(numbasedir,"here"),0,nummmaplen))
		numdatas.push_back(numdata);
		
	 if(Numdata* numdata=Numdata::getnumdata( pathconcat(numbasedir,"watch"),-1,nummmaplen)) {
		numdatas.push_back(numdata);
		}
	if(!backup)  {
		fprintf(stderr,"My error: No Backup\n");
		return 2;
		}
	if(argc==1)
		return 0;
	if(reinitpos>=0)
		backup->resethost(reinitpos) ;
	bool sender=	nums||scans||stream;
	int hostnr=backup->gethostnr();
	if(!sender&&!receive) {
		if(rmindex>=0) {
			if(rmindex>hostnr) {
				cerr<<"Argument to -R should refer to an existing connection (1-"<<hostnr<<endl<<")"<<endl;
				return 9;
				}
			backup->deletehost(rmindex);
			return 1234;
			}
		if(port.size()) {
			memcpy(backup->getupdatedata()->port,port.data(),port.size());
			backup->getupdatedata()->port[port.size()]='\0';

			did=true;
//			return 1234;
			}
	/*	if(unit) {
			did=true;
			} */
		if(list)
			return 	listconnections();
		if(clear)
			return clearhosts();
		if(numexport)  {
			 if(!exportnummer(numexport,starttime,endtime)) 
				return 13;
			did=true;
			}
#ifndef DONT_USE_LIBREVIEW
		if(showlibreview) {
			void makelibreviewdata() ;
			makelibreviewdata();

			did=true;
			}
#endif
		if(historyexport)  {
			 if(!exportdata(historyexport,exporthistory,starttime,endtime))
				return 13;
			did=true;

			}
		if(scanexport) {
			if(!exportdata(scanexport,exportscans<true>, &SensorGlucoseData::scanInperiod,starttime,endtime))
				return 13;
			did=true;
			}
		if(pollexport) {
			if(!exportdata(pollexport,exportscans<false>, &SensorGlucoseData::streamInperiod,starttime,endtime))
				return 13;
			did=true;
			}
		if(mealexport) {
			if(!exportdata(mealexport,allsavemeals,starttime,endtime))
				return 13;
			did=true;
			}
		if(detect||signal) {
			 help(argv[0]); 
			 return 0;
			}
		if(did)
			return 1234;
		return 0;
		}
	else {
           if (activeonly&&optind >= argc) {
               cerr<< "No IPS specified\n";
	       return 5;
           	}
 	uint32_t starttime=0;
	if(int er=backup->changehost(changer<0?hostnr:changer,nullptr,reinterpret_cast<jobjectArray>(argv+optind ),argc-optind, detect,port,nums,stream,scans,false,receive,activeonly,password,starttime,passiveonly,label,testip,false) <0) {
		cerr<<"changehost failed\n";
		return er;
		}
		}
	return 1234;
       }
#include "net/netstuff.h"
void sighandler(int sig) { }
static void wakeup() {
	backup->getupdatedata()->wakesender();
	backup->wakebackup(Backup::wakeall);
	//	backup->wakebackup();
	}
void exitproc() {
	cout<<"This is a normal exit"<<endl;
	}
 unsigned int alarm(unsigned int seconds);
// #include "SensorGlucoseData.h"
static	void setalarm();
int main(int argc,char **argv) {
//	bool active=backup->getupdatedata()->port[0];
//	if(active) backup->stopreceiver();
//	backup->changehost(0,"192.168.1.69","8795", false,false,false,false,true,true,"1234567890123456");
//	SensorGlucoseData test("/tmp/sensors/E07A-XX09K9HHHAJ",5);
	if(int ret=readconfig(argc,argv)) {
		if(ret==1234)
			return 0;
		return ret;
		}
	networkpresent=true;
	signal(SIGUSR1,sighandler);
	atexit(exitproc);
	setalarm();


	if(settings->data()->usexdripwebserver) {
		void startwatchthread() ;
		startwatchthread();
		}
//extern void	sendlibre3viewdata();
//	sendlibre3viewdata();
//void	getlibre3puttext(const char *);
//getlibre3puttext("0APDD6XTW");
	while(true) {
		wakeup();
		pause();
		perror("Got signal");
		}
	}

//int changehost(int index,string_view name,string_view port,const bool sendnums,const bool sendstream,const bool sendscans,const bool restore,const bool receive,const bool reconnect,string_view pass) 
void render() {
//  wakeup();
}

extern void     processglucosevalue(int sendindex,int newstart) ;
void     processglucosevalue(int sendindex,int newstart) {
	constexpr int maxbluetoothage=3*60;
	if(!sensors)
		return;
	LOGGER("processglucosevalue %d %d\n", sendindex,newstart);
	if(SensorGlucoseData *hist=sensors->getSensorData(sendindex)) {
		if(newstart>=0) {
			LOGGER("newstart=%d\n",newstart);
			hist->backstream(newstart);
			}
		if(const ScanData *poll=hist->lastpoll()) {
			const time_t nutime=time(nullptr);
			const time_t tim=poll->t;
			const int dif=nutime-tim;
			if(dif<maxbluetoothage) {
				sensor *senso=sensors->getsensor(sendindex);
				logprint("finished=%d not finished %s ",senso->finished,ctime(&tim));
				if(senso->finished) {
					senso->finished=0;
					backup->definished(sendindex);
					}

				}
			else {
				logprint(" too old %s ",ctime(&tim));
				logprint("dist=%d, dif=%d nu %s",maxbluetoothage,dif,ctime(&nutime));
				}
			}
		else {
			logprint("no stream data\n");
			}
		}
	else {
		logprint("no sensor\n");
		}
	}
#include "alarm.cpp"
/*
__asm__(".symver stat,stat@GLIBC_2.31");
__asm__(".symver fstat,fstat@GLIBC_2.31");
__asm__(".symver __libc_start_main,__libc_start_main@GLIBC_2.31");
__asm__(".symver __pthread_key_create,__pthread_key_create@GLIBC_2.31");
__asm__(".symver pthread_create,pthread_create@GLIBC_2.31");
__asm__(".symver _ZSt28__throw_bad_array_new_lengthv,_ZSt28__throw_bad_array_new_lengthv@GLIBCXX_3.4.28");

__asm__(".symver stat,stat@GLIBC_2.33");
__asm__(".symver fstat,fstat@GLIBC_2.33");
__asm__(".symver __libc_start_main,__libc_start_main@GLIBC_2.34");
__asm__(".symver __pthread_key_create,__pthread_key_create@GLIBC_2.34");
__asm__(".symver pthread_create,pthread_create@GLIBC_2.34");
__asm__(".symver _ZSt28__throw_bad_array_new_lengthv,_ZSt28__throw_bad_array_new_lengthv@GLIBCXX_3.4.29");

*/
extern void usepath();
void usepath(){}
extern bool getpathworks();
bool getpathworks() {
	return false;
	}
bool updateDevices() {
	LOGSTRING("updateDevices() called\n");
	return true;
	}
extern bool hour24clock;
bool hour24clock=true;

struct jugglucotext {
char daylabel[7][5];
char monthlabel[12][5];
};
extern jugglucotext engtext;
jugglucotext engtext {
        .daylabel={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"},
        .monthlabel={
      "Jan","Feb","Mar","Apr","May"      ,             "Jun",
       "Jul","Aug","Sep",
      "Oct","Nov","Dec"}
      };
void setfloatptr() {
        }

/*
extern std::string_view dRELEASE;
extern std::string_view dMANUFACTURER;
extern std::string_view dMODEL;
 std::string_view dRELEASE="RELEASE";
std::string_view dMANUFACTURER="MANUFACTURER";
 std::string_view dMODEL="MODEL";
 */

