
#ifndef WEAROS
#include <algorithm>
#include <jni.h>
#include "settings/settings.h"
#include "nums/numdata.h"
#include "logs.h"
#include "destruct.h"
constexpr const int treatmentitemsize=300;

constexpr const int	maxtreatments=100;
extern vector<Numdata*> numdatas;

extern bool nightuploadTreatments(const char *data,int len) ;
char *writetreatment(char *outiter,const int numbase,const int pos,const Num*num,int border);
extern  const int nighttimeback;


static jstring  makeurl(JNIEnv *env,std::string_view pathstr1,std::string_view pathstr2) {
		const int namelen=settings->data()->nightuploadnamelen;
		const char *name=settings->data()->nightuploadname;
		char fullname[namelen+pathstr1.size()+pathstr2.size()+1];
		memcpy(fullname,name,namelen);
		char *ptr=fullname+namelen;
		memcpy(ptr,pathstr1.data(),pathstr1.size());
		ptr+=pathstr1.size();
		memcpy(ptr,pathstr2.data(),pathstr2.size());
		ptr[pathstr2.size()]='\0';
		auto local=env->NewStringUTF(fullname);
		return local;
		}

extern jclass nightpostclass;

extern int mkid(char *outiter,int base,int pos) ;

extern jstring jnightuploadsecret;
bool deletetreatment(JNIEnv *env,int base,int del) {
	const static jmethodID  deleteUrl=env->GetStaticMethodID(nightpostclass,"deleteUrl","(Ljava/lang/String;Ljava/lang/String;)Z");
	constexpr const std::string_view treatment=R"(/api/v1/treatments/)";
	char buf[30];
	int len=mkid(buf, base, del);
	jstring url=makeurl(env,treatment,std::string_view(buf,len));
	LOGGER("delete %.24s\n",buf);
	bool res=env->CallStaticBooleanMethod(nightpostclass,deleteUrl,url,jnightuploadsecret);
	env->DeleteLocalRef(url);
	return res;
	}

extern JNIEnv *getenv();
bool uploadtreatments() {
	if(settings->data()->postTreatments) {
		const uint32_t oldtime=time(nullptr)-nighttimeback;
		const int basecount=numdatas.size();
		for(int base=0;base<basecount;++base) {
			auto *numdata=numdatas[base];
			auto last=numdata->getlastpos();
			auto start=numdata->getnightStart(); 
			auto send=numdata->getnightSend(); 
			const Num *nummers=numdata->startdata();
			if(send>start) {
				LOGGER("delete %d cases\n",send-start);
				auto env=getenv();
				for(int del=send-1;del>=start;--del) {
					if(!deletetreatment(env,base,del)) {
						LOGGER("deletetreatment(%d,%d) failed\n",base,del);
						return false;
						}
					numdata->getnightSend()=del;
					}
				LOGAR("deleted");
				}
			int nr=last-start;
			if(nr>0) {
					const Num *startnum=nummers+start;
					if(startnum->gettime()<oldtime) {
						 const Num zoek {.time=oldtime};
						 const Num *en= nummers+last;
						auto comp=[](const Num &el,const Num &se ){return el.time<se.time;};
						const Num *hit=std::upper_bound(startnum,en, zoek,comp);
						if(hit==en) {
							LOGGER("No new num after %d\n",start);
							return false;
							}
						start=hit-nummers;
						nr=last-start;
						}
					for(auto iter=start;iter<last;iter++) {
						const Num *num=nummers+iter;
						if(numdata->valid(num)) {
							char buf[treatmentitemsize];
							if(char *end=writetreatment(buf,base,iter,num,0);end!=buf) {
								int datalen=end-buf-1;
								logwriter(buf,datalen);
								if(nightuploadTreatments(buf,datalen)) {
									LOGGER("treatment base=%d len=%d nightupload Success\n",base,datalen);
									}
								else {
									numdata->setNightSend(iter+1);
									return false;
									}
								}
							else {
								LOGGER("Skip iter=%d, type=%d\n",iter,num->type);
								}
							}
						}
					numdata->setNightSend(last);
					}

			}
		}
	return true;
	}

//R"({"timestamp":%u000,"eventType":"<none>","enteredBy":"Juggluco","notes":"Walk 7","carbs":%g,"insulin":%g,"created_at":"2024-01-06T11:31:03Z","_id":"%s"}
#endif
