
#ifndef WEAROS
#include <jni.h>
#include <charconv>
#include <algorithm>
#include "settings/settings.h"
#include "nums/numdata.h"
#include "logs.h"
#include "destruct.h"
#include "nightnumcategories.h"
#include "common.h"
constexpr int HTTP_OK=200;

constexpr const int treatmentitemsize=300;

extern vector<Numdata*> numdatas;

extern int nightuploadTreatments(const char *data,int len) ;
extern int nightuploadTreatments3(const char *data,int len) ;
char *writetreatment(char *outiter,const int numbase,const int pos,const Num*num,int border,int borderID);
extern  const int nighttimeback;
int mkidV3(char *outiter,int base,int pos) ;
int mkididV3(char *outiter,int base,int pos) ;

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
extern int mkidid(char *outiter,int base,int pos) ;

extern jstring jnightuploadsecret;
bool deletetreatment(JNIEnv *env,int base,int del,int borderID) {
	const static jmethodID  deleteUrl=env->GetStaticMethodID(nightpostclass,"deleteUrl","(Ljava/lang/String;Ljava/lang/String;)Z");
	constexpr const std::string_view treatment=R"(/api/v1/treatments/)";
	char buf[30];
//	int len=mkid(buf, base, del);
	int len=(del>=borderID?mkidid:mkid)(buf, base, del);
	jstring url=makeurl(env,treatment,std::string_view(buf,len));
	LOGGER("delete %.24s\n",buf);
	bool res=env->CallStaticBooleanMethod(nightpostclass,deleteUrl,url,jnightuploadsecret);
	env->DeleteLocalRef(url);
	return res;
	}

bool deletetreatment3(JNIEnv *env,int base,int del,int borderID) {
	const static jmethodID  deleteUrl=env->GetStaticMethodID(nightpostclass,"deleteUrl","(Ljava/lang/String;Ljava/lang/String;)Z");
	constexpr const std::string_view treatment=R"(/api/v3/treatments/)";
	char buf[50];
	int len=(del>=borderID?mkididV3:mkidV3)(buf, base, del);
	jstring url=makeurl(env,treatment,std::string_view(buf,len));
	LOGGER("delete %.24s\n",buf);
	bool res=env->CallStaticBooleanMethod(nightpostclass,deleteUrl,url,jnightuploadsecret);
	env->DeleteLocalRef(url);
	return res;
	}

extern JNIEnv *getenv();

static char *writetreatmentV3(char *outiter,const int numbase,const int pos,const Num*num,int borderID) ;
static bool sendtreatment3(int base,int iter,const Num *num,Numdata *numdata) {
	char buf[treatmentitemsize];
	if(char *end=writetreatmentV3(buf,base,iter,num,numdata->getnightIDstart());end!=buf) {
		int datalen=end-buf;
		logwriter(buf,datalen);
		if(int res=nightuploadTreatments3(buf,datalen);res==200||res==201) {
			LOGGER("treatment base=%d len=%d nightupload Success\n",base,datalen);
			numdata->setNightSend(iter+1);
			return true;
			}
		else  {
			LOGGER("treatment base=%d len=%d nightupload failed code=%d\n",base,datalen,res);
			return false;
			}
		}
	else {
		LOGGER("Skip iter=%d, type=%d\n",iter,num->type);

		}
	return true;
	}

static bool sendtreatment(int base,int iter,const Num *num,Numdata *numdata) {
	char buf[treatmentitemsize];
	if(char *end=writetreatment(buf,base,iter,num,0,numdata->getnightIDstart());end!=buf) {
		int datalen=end-buf-1;
		logwriter(buf,datalen);
		if(nightuploadTreatments(buf,datalen)==HTTP_OK) {
			LOGGER("treatment base=%d len=%d nightupload Success\n",base,datalen);
			numdata->setNightSend(iter+1);
			return true;
			}
		return false;
		}
	else {
		LOGGER("Skip iter=%d, type=%d\n",iter,num->type);

		}
	return true;
	}
bool uploadtreatments(bool useV3) {
	if(settings->data()->postTreatments) {
		const uint32_t now=time(nullptr);
		const uint32_t oldtime=now-nighttimeback;
		const uint32_t lastloadtime=settings->data()->lastuploadtime;
		LOGGER("lastloadtime=%d\n",lastloadtime);
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
					 if(numdata->changedsince(lastloadtime,del)||del>=last) {		
						if(!((useV3?deletetreatment3:deletetreatment)(env,base,del,numdata->getnightIDstart()))) {
							LOGGER("deletetreatment(%d,%d) failed\n",base,del);
							return false;
							}
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
							continue;
							}
						start=hit-nummers;
						nr=last-start;
						}
					for(auto iter=start;iter<last;iter++) {
						const Num *num=nummers+iter;
						if(numdata->valid(num)) {
					 		if(numdata->changedsince(lastloadtime,iter)||num->gettime()>=lastloadtime) {		
								if(!(useV3?sendtreatment3:sendtreatment)(base,iter,num,numdata) ) {
									return false;
									}
								}
							}
						}
					numdata->setNightSend(last);
					}

			}

		settings->data()->lastuploadtime=now;
		LOGGER("new lastloadtime=%d\n",now);
		}
	return true;
	}


char *writetreatmentV3(char *outiter,const int numbase,const int pos,const Num*num,int borderID) {
	const int type=num->type;
	if(type>=settings->varcount()||!settings->data()->Nightnums[type].kind) {
		return outiter;
		}
	const time_t tim=num->gettime();
	addar(outiter,R"({"identifier":")");

	outiter+= (pos>=borderID?mkididV3:mkidV3)(outiter,numbase,pos);
	addar(outiter,R"(","_id":")");
	outiter+=(pos>=borderID?mkidid:mkid)(outiter,numbase, pos) ;
	addar(outiter,R"(","date":)");
	if(auto [ptr,ec]=std::to_chars(outiter,outiter+15,tim);ec == std::errc()) {
		outiter=ptr;
		}
	else
		LOGGER("tochar failed: %s\n",std::make_error_code(ec).message().c_str());

	addar(outiter,R"(000,"utcOffset":0,"eventType":"<none>","app":"Juggluco",)");
  

	float w=0.0f;
	 if((w=longNightWeight(type))!=0.0f) {
	 	
	 	addar(outiter,R"("notes":"Long-Acting",)");
	 	}
	else { if((w=rapidNightWeight(type))!=0.0f) {
	 	addar(outiter,R"("notes":"Rapid-Acting",)");
	 	}
		}
	if(w!=0.0f) {
		const char * typestr=settings->getlabel(type).data();;
		auto units=w*num->value;
		outiter+=sprintf(outiter,R"("insulin":%g,"insulinType":"%s"})",units,typestr);

		}
	else {
		if((w=carboNightWeight(type) )!=0.0f) {
			outiter+=sprintf(outiter,R"("carbs":%g})",w*num->value);
			}
		else {
			std::string_view typestr=settings->getlabel(type);
			outiter+=sprintf(outiter,R"("notes":"%s %g"})",typestr.data(),num->value);
			}
		}
	return outiter;
	}
//R"({"timestamp":%u000,"eventType":"<none>","enteredBy":"Juggluco","notes":"Walk 7","carbs":%g,"insulin":%g,"created_at":"2024-01-06T11:31:03Z","_id":"%s"}
#endif
