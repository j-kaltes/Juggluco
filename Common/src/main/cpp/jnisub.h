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
/*      Fri Jan 27 12:35:09 CET 2023                                                 */


#ifndef JNISUB_H
#define JNISUB_H
#include <string_view>
#include <utility>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <jni.h>
#include <filesystem>
#include <vector>
#include "logs.h"
#include "inout.h"
#include "serial.h"
#include "nfcdata.h"
#include "sensoren.h"
#define VISIBLE __attribute__((__visibility__("default")))

extern Sensoren *sensors;
extern jint parsertype;
struct jniHistory;
struct GlucoseValue;
#include "glucose.h"
//typedef std::vector<GlucoseValue*> jniHistory;
struct scandata;
struct jniHistory:std::vector<GlucoseValue*> {
	const GlucoseValue &get(int i) const {return *operator[](i);}
	}; /*How to use a forward declaration in jnisub.h without mentioning vector?*/
struct AlgorithmResults {
    const jniHistory *hist;
    const GlucoseNow *glu;
/*    const int trendArrow;
    const float rateOfChange;
    */
     const int alarm;
    const jboolean isActionable;
    const int esaMinutesToWait;
    const jboolean lsaDetected;
    bool inserted,removed;
    void setremoved(int rem) {
    	removed=rem;
    	}
    void setinserted(int ins) {
    	inserted=ins;
    	}

void showresults(FILE *stream,scandata *dat) const;
    const jniHistory &history() const {return *hist;}
   const GlucoseNow &currentglucose() const {
	return *glu;
	}
   const jboolean getlsaDetected() const {
   	return lsaDetected;
   	}
   int	trend() const {
	return glu->trendArrow;
	}
  float	RateOfChange() const {
    	return  glu->rateOfChange;
	}
~AlgorithmResults();
};

using namespace std;
struct VISIBLE scandata  ;
struct scandata: public nfcdata {
	bool error=false;
//	time_t nutime;
	data_t *data,*info;
  scandata(std::string_view target) ;
  scandata(std::filesystem::path &pad) : scandata(pad.native()) {}
  scandata(time_t tim,const data_t *datain,const data_t *info):  data(data_t::newex(datain)),info(data_t::newex(info)) {
		settime(tim);
		setdata(data->data());
		}
//  scandata(time_t tim,const std::array<const signed char,344> &raw,const std::array<const signed char,6> &info):  data(data_t::newex(raw)),info(data_t::newex(info)) {
  scandata(time_t tim,const std::array<const uint8_t,344> &raw,const std::array<const uint8_t,6> &info):  data(data_t::newex(raw)),info(data_t::newex(info)) {
		settime(tim);
		setdata(data->data());
		}
  scandata( JNIEnv *env,jbyteArray info,jbyteArray dat,time_t tim=time(nullptr));
  ~scandata(); 
	};
#include "scanstate.h"

struct outobj;

template <class B,class M,class ...Ts>
B * locknew(M &mu,Ts && ... args) {
	LOGSTRING("locknew\n");
        std::lock_guard<M> lock(mu);
        return new B(std::forward<Ts>(args)...);
        }



class Abbott {
public:
struct scanresult_t {
	const AlgorithmResults *res;
	const ScanData *scan;
    } ;
string serial;
int sensorindex=sensors->sensorindex(serial.data());
SensorGlucoseData *hist=sensorindex<0?nullptr:sensors->getSensorData(sensorindex);

private:
pathconcat sensordir;
data_t *uid;
scanstate *state;
int errorcode=0;
//Sensoren sens;

//Abbott(string_view basedir,string &&sensor,data_t *uidin): serial(std::move(sensor)),sensordir(basedir,serial),uid(uidin),state(hist?(hist->mutex.lock(),new scanstate(sensordir)):nullptr) {
Abbott(string_view basedir,string &&sensor,data_t *uidin): serial(std::move(sensor)),sensordir(basedir,serial),uid(uidin),state(hist?locknew<scanstate>(hist->mutex,sensordir):nullptr) {
//	if(hist) hist->mutex.unlock();
	LOGSTRING("Abbott(string_view basedir,string &&sensor,data_t *uidin)\n");
	std::filesystem::path dir( sensordir.cbegin(),sensordir.cend());
	 if(!((is_directory(dir)||std::filesystem::create_directories(dir) )&&!access(sensordir.data(), W_OK|R_OK|X_OK))) {
		lerror(sensordir.data());
		errorcode=1;
	 	}
	}
/*
Abbott(string_view basedir,data_t *uidin,int fam=0): sensordir(basedir,getserial(fam,reinterpret_cast<unsigned char *>(uidin->data()))), uid(uidin) {
	init();
	}
	*/
Abbott(string_view basedir,data_t *uidin,int fam): Abbott(basedir,getserial(fam,reinterpret_cast<unsigned char *>(uidin->data())), uidin) {
	}
public:
const string_view getsensordir() const {return sensordir;};
const data_t * getsensorid() const {return uid;};
/*
Abbott(string_view basedir,string_view sensor): sensordir(basedir,sensor), uid(data_t::newex(unserial(sensor.data()))) {
	init();
	}
	*/

Abbott(string_view basedir,string_view  sensor): Abbott(basedir,string(sensor), data_t::newex(unserial(sensor.data()))) {
	}

//Abbott(JNIEnv *env,jobject obj,string_view basedir,jbyteArray juid, jbyteArray info);
Abbott(JNIEnv *env,string_view basedir,jbyteArray juid, jbyteArray info);
~Abbott() {
 	data_t::deleteex(uid);
	delete state;
	}
int error() const  {return errorcode;}
scanresult_t ProcessOne(scandata *data)  ;
AlgorithmResults *callAbbottAlg(int startsincebase,scanstate *state,scandata *data,scanstate *newstate,outobj *starttime,outobj *endtime,jobject person) ;
//AlgorithmResults *callAbbottAlg(scanstate *state,scandata *data,scanstate *newstate) ;
scanresult_t callAbbottAlg(scandata *data) ;
scanstate *initnewsensor( scandata *data) ;

};
//AlgorithmResults *callAbbottAlg(ByteArray *uid,scanstate *state,scandata *data,scanstate *newstate);
//extern "C" JNIEXPORT jint JNICALL Java_tk_glucodata_Glucose_init(JNIEnv *env, jclass thiz,jstring dir) ;

//extern scanstate *initnewsensor( scandata *data, ByteArray *uid) ;
//int  abbottinit(std::string_view filesdir,JNIEnv *env=nullptr,jobject thiz=nullptr);
extern int  abbottinit(bool doch=false);

bool linklib(const char *filename) ;

typedef int8_t* Bytes_t;

struct timevalues {
int warmup,wear;
};

extern timevalues     patchtimevalues(const data_t *info) ;
extern jbyte  getactivationcommand(const data_t *info) ;
//extern data_t  * getactivationpayload(const data_t * patchid, const data_t * info, jbyte person) ;
data_t  * getactivationpayload(scanstate *state,const data_t * patchid, const data_t * info, jbyte person) ;

extern const ScanData *lastscan;

constexpr time_t basesecs=1262304000L;
constexpr const int endsensorsecs= (14*24-1)*60*60; 
constexpr const int days15=(15*24*60*60);
#include "secs.h"
struct streamdata {
	int libreversion;
	int sensorindex;
	SensorGlucoseData *hist;
	streamdata(int libreversion,int sensorindex,SensorGlucoseData *sens):libreversion(libreversion),sensorindex(sensorindex),hist(sens) {}
	streamdata(int libreversion,int sensorindex):streamdata(libreversion,sensorindex,sensors->getSensorData(sensorindex)) {}
	streamdata(int libreversion,const char *sensorname):streamdata(libreversion,sensors->sensorindex(sensorname)) {}
	virtual bool good() const {
		return true;
		};
	virtual ~streamdata() {};

	};
struct libre3stream:streamdata {
	libre3stream(int sensindex,SensorGlucoseData *sens): streamdata(3, sensindex,sens){};
	};

struct AlgorithmContext;
struct sistream:streamdata {
	AlgorithmContext *algcontext;
	sistream(int sensindex,SensorGlucoseData *sens,AlgorithmContext *context): streamdata(0x10, sensindex,sens),algcontext(context){
		};
	};

struct libre2stream:streamdata {
	int startsincebase;
	jbyte person=0;
	scanstate *state;
	#ifndef NORAWSTREAM
	int blueuit;
	#endif
public:
	libre2stream(int sensindex,SensorGlucoseData *sens):streamdata(2, sensindex,sens),startsincebase(((time(NULL)-hist->getstarttime())>=endsensorsecs?daysecs:0)+hist->getstarttime()-basesecs),state(locknew<scanstate>(hist->mutex,hist->getsensordir())) {
#ifndef NORAWSTREAM
		pathconcat uit(hist->getsensordir(),rawstream);
		blueuit=open(uit.data(),O_APPEND|O_CREAT|O_WRONLY, S_IRUSR |S_IWUSR);
#endif

//		hist->mutex.unlock();

		}




	const data_t *getident() const {
		return hist->getsensorident();
		}
	const data_t *getinfo() const {
		return hist-> getpatchinfo(); 
		}
~libre2stream() {
#ifndef NORAWSTREAM
	close(blueuit);
#endif
	LOGAR("~libre2stream()"); 
     delete state;
	}
bool good() const override {
	return state&&state->good();
	}
void setstate(scanstate *newstate) {	
	delete state;
	state=newstate;
	}
void renewstate() {
	scanstate *tmpstate= state;
	state= new scanstate(hist->getsensordir());
	delete tmpstate;
	}

const AlgorithmResults *  processTooth(data_t * bluedata, scanstate *newstate,uint32_t nutime) ;
};

extern pathconcat sensorbasedir;
inline  static  const AlgorithmResults  __attribute__((unused)) *Initialized=reinterpret_cast<const AlgorithmResults *>(1ul);
#define ALGDUP_VALUE Initialized
//constexpr static  const AlgorithmResults * Initialized=(const AlgorithmResults *)(1ul);


data_t *fromjbyteArray(JNIEnv *env,jbyteArray jar,jint len=-1);
class MemoryRegion;

#ifdef NFCMEM
struct NfcMemory {
static constexpr const int len=344;
data_t *uid;
data_t *info;
data_t *results=data_t::newex(len);
MemoryRegion *prevspan=nullptr;
int iter;
NfcMemory(JNIEnv *env,jbyteArray juid, jbyteArray jinfo): uid(fromjbyteArray(env,juid)), info(fromjbyteArray(env,jinfo)),iter(0) {
	};
int nextspan();
void add(data_t *dat) ;

~NfcMemory() {
 	data_t::deleteex(uid);
 	data_t::deleteex(info);
 	data_t::deleteex(results);
	};
};
#endif

#define WAS_JNIEXPORT 
typedef WAS_JNIEXPORT jobject JNICALL   (*oldprocessStream_t)(JNIEnv *envin, jobject obj, 
jint parsertype, jobject alarmconf, jobject nonaction, jobject glrange, jobject attenuatinconfig, jbyteArray sensorident, 
jbyte person, jbyteArray bluetoothdata, 
jint startsincebase,jint nusincebase,jint warmup,jint wear,
jbyteArray compostate, jbyteArray attenustate, jbyteArray measurestate,
jobject outstarttime, jobject endtime, jobject confinsert, jobject removed, 
jobject compo, jobject attenu, jobject messstate, 
 jobject algorithresults);
#endif
