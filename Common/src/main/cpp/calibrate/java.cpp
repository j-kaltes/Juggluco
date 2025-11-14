
#include <math.h>
#include "SensorGlucoseData.hpp"
#include "fromjava.h"
//#include "streamdata.hpp"


extern "C" JNIEXPORT jint JNICALL   fromjava(calibrateNR)(JNIEnv *env, jclass cl,jlong sensorptr) {
    if(!sensorptr)
        return 0;
    return reinterpret_cast<const SensorGlucoseData*>(sensorptr)->getinfo()->caliNr;
    }
extern "C" JNIEXPORT jboolean JNICALL   fromjava(getCalibrator)(JNIEnv *env, jclass cl,jlong sensorptr,jint pos,jobject jcalip) {
    if(!sensorptr)
        return false;
    const auto *info=reinterpret_cast<const SensorGlucoseData*>(sensorptr)->getinfo();
    const int caliNr=info->caliNr;
    if(pos>=caliNr)
        return false;

    static jclass jcaliC=env->GetObjectClass(jcalip);
    static jfieldID jtime=env->GetFieldID(jcaliC,"time","J");
    static jfieldID ja=env->GetFieldID(jcaliC,"a","D");
    static jfieldID jb=env->GetFieldID(jcaliC,"b","D");


    const CaliPara &item = info->caliPara[pos];

    env->SetLongField(jcalip,jtime,item.time*1000LL);
    env->SetDoubleField(jcalip,ja,item.a);
    env->SetDoubleField(jcalip,jb,gconvert(item.b*10));
    return true;
    }
extern void setCalibrates(uint16_t sensorindex) ;
extern "C" JNIEXPORT jboolean JNICALL   fromjava(removeCalibrator)(JNIEnv *env, jclass cl,jlong sensorptr,jint pos) {
    if(!sensorptr)
        return false;
    auto *sensor=reinterpret_cast<SensorGlucoseData*>(sensorptr);
    if(sensor->getinfo()->removeCaliPos(pos)) {
        setCalibrates(sensor->sensorIndex);
        return true;
        }
    return false;
    }

extern bool shouldexclude(const uint32_t time);
 extern "C" JNIEXPORT jboolean JNICALL   fromjava(shouldExclude)(JNIEnv *env, jclass cl,jlong msec) {
        uint32_t time=msec/1000;
        bool should= shouldexclude(time);
       LOGGER("shouldExclude(%u)=%d\n",time,should);
        return should;
        }

