// SPDX-License-Identifier: GPL-3.0-or-later
#include "store.hpp"
#include "nums/numdata.hpp"
#include "curve/numdisplay.hpp"
#include "curve/numhit.hpp"
#include <jni.h>
#include <memory>
#include <sstream>
#include <iomanip>

extern std::vector<Numdata*> numdatas;
extern std::string_view globalbasedir;
namespace ja=juggluco::activity;
namespace {
ja::Store &store() {
    if(!settings||globalbasedir.empty())throw std::runtime_error("Juggluco data is not initialized");
    static ja::Store s{std::string(globalbasedir)};return s;
}
struct Text {
    JNIEnv *env; jstring str; const char *p;
    Text(JNIEnv *e,jstring s):env(e),str(s),p(s?e->GetStringUTFChars(s,nullptr):nullptr) {}
    ~Text(){if(p)env->ReleaseStringUTFChars(str,p);}
    operator std::string_view() const {return p?std::string_view(p):std::string_view{};}
};
void error(JNIEnv *env,const std::exception &e) {
    if(!env->ExceptionCheck()) env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),e.what());
}
bool allowedLabel(int n) {
    return n>=0&&n<settings->getlabelcount()&&n!=settings->data()->bloodvar&&
        n!=settings->data()->mealvar&&settings->getIOBtype(n)==Insulin::Not;
}
int defaultLabel(ja::Store &s) {
    int n=s.header().defaultLabel;
    if(allowedLabel(n)) return n;
    if(settings->getlabelcount()>=maxvarnr) return -1;
    // Do not silently assign semantics to an existing user label with this name.
    n=settings->getlabelcount();
    settings->setlabel(n,"Activity",1.0f,0.0f);
    settings->data()->insulintypes[n]=Insulin::Not;
    settings->data()->librenums[n]={0,0}; settings->data()->Nightnums[n]={0,0};
    s.header().defaultLabel=n;
    return n;
}
struct Found { Numdata *data=nullptr; int pos=-1; };
Found findNumber(uint32_t group) {
    for(auto *d:numdatas) if(!d->receivedbackup()) for(int p=d->getfirstpos();p<d->getlastpos();++p) {
        const auto &n=d->at(p);
        if(d->valid(&n)&&n.type!=settings->data()->bloodvar&&n.mealptr==(Num::sportbit|group)) return {d,p};
    }
    return {};
}
void noteOverrides(ja::Record &r,const Num &n) {
    if(!(r.flags&ja::Generated)) return;
    if(n.type!=uint32_t(r.generatedLabel)) r.flags|=ja::LabelOverride;
    if(n.value!=r.generatedValue) r.flags|=ja::ValueOverride;
    if(n.time!=r.generatedTime) r.flags|=ja::TimeOverride;
}
void associate(ja::Store &s,uint32_t group,bool relabel=false) {
    auto *root=s.get(group); if(!root||!s.valid(*root)) return;
    auto rule=s.rule(root->type);
    if(!allowedLabel(rule.label)) rule.label=defaultLabel(s);
    auto found=findNumber(group);
    if(relabel&&!found.data) return;
    const int metric=found.data&&(root->flags&ja::Generated)?root->generatedMetric:rule.metric;
    const ja::Record *measurement=nullptr;
    for(uint32_t i=1;i<=s.header().count;++i) {
        auto *r=s.get(i);
        if(r->group==group&&!(r->flags&(ja::Deleted|ja::NeedsDetails|ja::NotReturned))&&s.valid(*r)) {
            if(!measurement||(!std::isfinite(ja::Store::amount(*measurement,metric))&&
                              std::isfinite(ja::Store::amount(*r,metric)))) measurement=r;
        }
    }
    if(!measurement) return; // Source deletion never deletes a user's Amount.
    const float value=ja::Store::amount(*measurement,rule.metric);
    if(found.data) {
        auto n=found.data->at(found.pos);
        noteOverrides(*root,n);
        root->flags|=ja::Linked;
        if(root->flags&ja::Generated) {
            int label=n.type; float amount=n.value;uint32_t time=n.time;
            if(relabel&&!(root->flags&ja::LabelOverride)&&allowedLabel(rule.label)) label=rule.label;
            // A changed mapping measure affects future entries. Existing amounts
            // retain their original unit, even when their source is corrected.
            float updated=ja::Store::amount(*measurement,root->generatedMetric);
            if(!(root->flags&ja::ValueOverride)&&std::isfinite(updated)) amount=updated;
            // Relabeling can be opened above an Amount editor, so defer any
            // change that moves the number array until normal linking resumes.
            if(!relabel&&!(root->flags&ja::TimeOverride))time=root->start;
            if(label!=int(n.type)||amount!=n.value||time!=n.time) {
                found.data->numchange(&found.data->at(found.pos),time,amount,label,n.mealptr);
                root->generatedLabel=label; root->generatedValue=amount;root->generatedTime=time;
            }
        }
        return;
    }
    // Remember deliberate deletion of a linked Amount; do not resurrect it.
    if(root->flags&ja::Linked) return;
    Found candidate; int candidates=0;
    for(auto *d:numdatas) {
        if(d->receivedbackup()) continue;
        for(auto *n=d->firstnotless(root->start);n<d->end()&&n->time<=root->end;++n) {
            if(d->valid(n)&&n->type==uint32_t(rule.label)&&n->mealptr==0) {
                candidate={d,d->index(n)}; ++candidates;
            }
        }
    }
    if(candidates>1) {root->flags|=ja::Ambiguous;return;}
    root->flags&=~ja::Ambiguous;
    if(candidates==1) {
        const Num n=candidate.data->at(candidate.pos);
        candidate.data->numchange(&candidate.data->at(candidate.pos),n.time,n.value,n.type,Num::sportbit|group);
        root->flags|=ja::Linked;
        return;
    }
    if(!allowedLabel(rule.label)||!std::isfinite(value)) return;
    for(auto *d:numdatas) if(d->getindex()==1&&!d->receivedbackup()) {
        // Record ownership before insertion, so replay recognizes its origin.
        root->generatedLabel=rule.label; root->generatedMetric=rule.metric;
        root->generatedValue=value; root->generatedTime=root->start; root->flags|=ja::Generated;
        d->numsave(root->start,value,rule.label,Num::sportbit|group);
        if(findNumber(group).data) root->flags|=ja::Linked;
        break;
    }
}
}

#define AJ(name) Java_tk_glucodata_ActivityNative_##name
#define BEGIN try { auto &s=store(); std::lock_guard guard(s.mutex);
#define END } catch(const std::exception &e) { error(env,e); }
extern "C" JNIEXPORT jlongArray JNICALL AJ(config)(JNIEnv *env,jclass) {
    BEGIN
    auto &h=s.header(); jlong v[]={h.from,h.interval,h.lastSuccess,h.defaultLabel,h.generation};
    auto out=env->NewLongArray(5); env->SetLongArrayRegion(out,0,5,v); return out;
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(configure)(JNIEnv *env,jclass,jlong from,jint interval,jint label) {
    BEGIN
    if(from<0||from>UINT32_MAX||interval<0||(interval>0&&interval<900)) throw std::invalid_argument("Invalid retrieval settings");
    if(label>=0&&!allowedLabel(label)) throw std::invalid_argument("Choose an activity label");
    if(s.header().from!=uint32_t(from)) {s.resetTokens();++s.header().generation;s.header().lastSuccess=0;}
    s.header().from=from;s.header().interval=interval;s.header().defaultLabel=label;
    END
}
extern "C" JNIEXPORT jstring JNICALL AJ(token)(JNIEnv *env,jclass,jint type) {
    BEGIN
    if(type<0||type>=16) throw std::out_of_range("Token type");
    return env->NewStringUTF(std::string(s.token(type)).c_str());
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(setToken)(JNIEnv *env,jclass,jint type,jstring token) {
    BEGIN Text t(env,token);s.token(type,t); END
}
extern "C" JNIEXPORT void JNICALL AJ(checkpoint)(JNIEnv *env,jclass,jint type,jstring token,jboolean dirty) {
    BEGIN Text t(env,token);s.checkpoint(type,t,dirty); END
}
extern "C" JNIEXPORT void JNICALL AJ(resetTokens)(JNIEnv *env,jclass) {BEGIN s.resetTokens();++s.header().generation;s.header().lastSuccess=0; END}
extern "C" JNIEXPORT void JNICALL AJ(beginSessionSnapshot)(JNIEnv *env,jclass) {BEGIN s.beginSessionSnapshot(); END}
extern "C" JNIEXPORT void JNICALL AJ(permissions)(JNIEnv *env,jclass,jint mask) {
    BEGIN if(s.header().readPermissions!=uint32_t(mask)) {
        s.resetTokens();++s.header().generation;s.header().lastSuccess=0;s.header().readPermissions=mask;
    } END
}
extern "C" JNIEXPORT void JNICALL AJ(succeeded)(JNIEnv *env,jclass,jlong now) {BEGIN s.header().lastSuccess=now; END}
extern "C" JNIEXPORT jint JNICALL AJ(source)(JNIEnv *env,jclass,jstring origin,jstring device) {
    BEGIN Text a(env,origin),b(env,device);return s.source(a,b); END return 0;
}
extern "C" JNIEXPORT jstring JNICALL AJ(sourceName)(JNIEnv *env,jclass,jint source) {
    BEGIN auto item=s.source(source);std::string name(s.text(item.origin));
    if(auto device=s.text(item.device);!device.empty()){name+=" · ";name+=device;}
    return env->NewStringUTF(name.c_str()); END return nullptr;
}
extern "C" JNIEXPORT jint JNICALL AJ(upsert)(JNIEnv *env,jclass,jstring id,jstring origin,jstring title,jstring notes,jlong start,jlong end,jint type,jlong modified,jint source) {
    BEGIN
    if(start<=0||end>UINT32_MAX||end<=start||type<0) throw std::invalid_argument("Invalid activity interval");
    Text a(env,id),b(env,origin),c(env,title),d(env,notes);
    ja::Record r{};r.start=start;r.end=end;r.type=type;r.modified=modified;r.source=source;
    return s.upsert(a,b,c,d,r);
    END return 0;
}
extern "C" JNIEXPORT void JNICALL AJ(deleted)(JNIEnv *env,jclass,jstring id) {
    BEGIN Text a(env,id);
    for(uint32_t i=1;i<=s.header().count;++i) if(s.text(s.get(i)->id)==std::string_view(a)) s.get(i)->flags|=ja::Deleted;
    END
}
extern "C" JNIEXPORT jintArray JNICALL AJ(indices)(JNIEnv *env,jclass,jboolean pending) {
    BEGIN std::vector<jint> list;
    for(uint32_t i=1;i<=s.header().count;++i) {auto &r=*s.get(i);
        if(s.valid(r)&&(pending?(!(r.flags&(ja::Deleted|ja::NotReturned))&&(r.flags&ja::NeedsDetails)&&r.start>=s.header().from):(r.group==i))) list.push_back(i);
    }
    std::sort(list.begin(),list.end(),[&](int a,int b){return s.get(a)->start>s.get(b)->start;});
    auto out=env->NewIntArray(list.size());if(!list.empty())env->SetIntArrayRegion(out,0,list.size(),list.data());return out;
    END return nullptr;
}
extern "C" JNIEXPORT jdoubleArray JNICALL AJ(record)(JNIEnv *env,jclass,jint index) {
    BEGIN auto *r=s.get(index);if(!r||!s.valid(*r))return nullptr;
    jdouble v[20]={double(r->group),double(r->flags),double(r->type),double(r->start),double(r->end),double(r->modified)};
    for(size_t i=0;i<ja::FieldCount;++i)v[6+i]=r->values[i];
    v[18]=r->generatedMetric;v[19]=r->generatedValue;
    auto out=env->NewDoubleArray(20);env->SetDoubleArrayRegion(out,0,20,v);return out;
    END return nullptr;
}
extern "C" JNIEXPORT jobjectArray JNICALL AJ(texts)(JNIEnv *env,jclass,jint index) {
    BEGIN auto *r=s.get(index);if(!r||!s.valid(*r))return nullptr;
    auto out=env->NewObjectArray(5,env->FindClass("java/lang/String"),nullptr);
    ja::Slice fields[]={r->id,r->origin,r->title,r->notes,s.source(r->source).device};
    for(int i=0;i<5;++i){auto text=env->NewStringUTF(std::string(s.text(fields[i])).c_str());env->SetObjectArrayElement(out,i,text);env->DeleteLocalRef(text);}return out;
    END return nullptr;
}
extern "C" JNIEXPORT jintArray JNICALL AJ(sources)(JNIEnv *env,jclass,jint group) {
    BEGIN std::vector<jint> list;
    for(uint32_t i=1;i<=s.header().count;++i)if(s.get(i)->group==uint32_t(group)&&s.valid(*s.get(i)))list.push_back(i);
    auto out=env->NewIntArray(list.size());if(!list.empty())env->SetIntArrayRegion(out,0,list.size(),list.data());return out;
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(metrics)(JNIEnv *env,jclass,jint index,jdoubleArray values) {
    BEGIN auto *r=s.get(index);if(!r||env->GetArrayLength(values)!=ja::FieldCount)throw std::invalid_argument("Activity metrics");
    jdouble v[ja::FieldCount];env->GetDoubleArrayRegion(values,0,ja::FieldCount,v);
    for(size_t i=0;i<ja::FieldCount;++i)r->values[i]=v[i];
    END
}
extern "C" JNIEXPORT void JNICALL AJ(samples)(JNIEnv *env,jclass,jint index,jint kind,jlongArray times,jfloatArray values,jintArray sources) {
    BEGIN
    const auto count=env->GetArrayLength(times);
    if(kind<0||kind>4||count!=env->GetArrayLength(values)||count!=env->GetArrayLength(sources))throw std::invalid_argument("Activity samples");
    std::vector<jlong> t(count);std::vector<jfloat> v(count);std::vector<jint> source(count);std::vector<ja::Sample> samples;samples.reserve(count);
    if(count){env->GetLongArrayRegion(times,0,count,t.data());env->GetFloatArrayRegion(values,0,count,v.data());env->GetIntArrayRegion(sources,0,count,source.data());}
    for(int i=0;i<count;++i) if(std::isfinite(v[i])&&v[i]>=0&&((kind!=0&&kind!=4)||v[i]>0))samples.push_back({t[i],v[i],uint32_t(source[i])});
    std::stable_sort(samples.begin(),samples.end(),[](auto &a,auto &b){return a.source==b.source?a.time<b.time:a.source<b.source;});
    samples.erase(std::unique(samples.begin(),samples.end(),[](auto &a,auto &b){return a.source==b.source&&a.time==b.time&&a.value==b.value;}),samples.end());
    s.samples(index,kind,samples);
    END
}
extern "C" JNIEXPORT jdoubleArray JNICALL AJ(heartSources)(JNIEnv *env,jclass,jint index) {
    BEGIN std::vector<jdouble> values;auto *r=s.get(index);
    if(r) {
        auto samples=s.view<ja::Sample>(r->heart);
        while(!samples.empty()) {
            auto selected=ja::Store::forSource(samples,samples.front().source);
            if(selected.empty())break;
            auto stats=ja::Store::statistics(selected,int64_t(r->start)*1000,int64_t(r->end)*1000);
            values.insert(values.end(),{double(samples.front().source),stats[0],stats[1],stats[2]});
            samples=samples.subspan(selected.size());
        }
    }
    auto out=env->NewDoubleArray(values.size());if(!values.empty())env->SetDoubleArrayRegion(out,0,values.size(),values.data());return out;
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(complete)(JNIEnv *env,jclass,jint index) {
    BEGIN auto *r=s.get(index);if(r)r->flags&=~ja::NeedsDetails; END
}
extern "C" JNIEXPORT void JNICALL AJ(linkAmounts)(JNIEnv *env,jclass) {
    BEGIN
    for(uint32_t i=1;i<=s.header().count;++i)if(s.get(i)->group==i)associate(s,i);
    if(backup)backup->wakebackup(wakenums);
    END
}
extern "C" JNIEXPORT jintArray JNICALL AJ(rule)(JNIEnv *env,jclass,jint type) {
    BEGIN auto r=s.rule(type);jint v[]={r.label,r.metric};auto out=env->NewIntArray(2);env->SetIntArrayRegion(out,0,2,v);return out; END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(setRule)(JNIEnv *env,jclass,jint type,jint label,jint metric,jboolean existing) {
    BEGIN if(!allowedLabel(label))throw std::invalid_argument("Choose an activity label");
    s.setRule({type,label,metric,0});
    if(existing)for(uint32_t i=1;i<=s.header().count;++i)if(s.get(i)->group==i&&s.get(i)->type==uint32_t(type))associate(s,i,true);
    if(backup)backup->wakebackup(wakenums);
    END
}
extern "C" JNIEXPORT jint JNICALL AJ(newLabel)(JNIEnv *env,jclass,jstring name) {
    BEGIN Text text(env,name);auto str=std::string_view(text);
    if(str.empty()||str.size()>11)throw std::invalid_argument("Label must contain 1 to 11 UTF-8 bytes");
    if(settings->getlabelcount()>=maxvarnr)throw std::runtime_error("No free labels; choose an existing label");
    const int n=settings->getlabelcount();settings->setlabel(n,std::string(str).c_str(),1.0f,0.0f);
    settings->data()->insulintypes[n]=Insulin::Not;settings->data()->librenums[n]={0,0};settings->data()->Nightnums[n]={0,0};return n;
    END return -1;
}
extern "C" JNIEXPORT jboolean JNICALL AJ(labelAllowed)(JNIEnv*,jclass,jint label) { return allowedLabel(label); }
extern "C" JNIEXPORT jint JNICALL AJ(reference)(JNIEnv *env,jclass,jlong hit) {
    BEGIN if(!hit)return 0;auto *numhit=reinterpret_cast<NumHit*>(hit);auto *n=numhit->hit;
    // Activity payloads are local. Do not interpret a mirrored device's index
    // as a record in this device's activity file.
    if(numhit->numdisplay->receivedbackup())return 0;
    if(!n||n->type==settings->data()->bloodvar||!Num::issportref(n->mealptr))return 0;
    auto id=ja::indexOf(n->mealptr);auto *r=s.get(id);return r&&s.valid(*r)?id:0;
    END return 0;
}
extern "C" JNIEXPORT jobjectArray JNICALL AJ(candidates)(JNIEnv *env,jclass,jint group) {
    BEGIN std::vector<std::string> list;auto *r=s.get(group);if(!r)return nullptr;
    for(auto *d:numdatas)if(!d->receivedbackup())for(auto *n=d->firstnotless(r->start);n<d->end()&&n->time<=r->end;++n)
        if(d->valid(n)&&n->mealptr==0&&allowedLabel(n->type)) {
            std::ostringstream text;text<<std::setprecision(std::numeric_limits<float>::max_digits10)<<d->getindex()<<':'<<d->index(n)<<':'<<n->time<<':'<<n->type<<':'<<n->value;
            list.push_back(text.str());
        }
    auto out=env->NewObjectArray(list.size(),env->FindClass("java/lang/String"),nullptr);
    for(size_t i=0;i<list.size();++i){auto t=env->NewStringUTF(list[i].c_str());env->SetObjectArrayElement(out,i,t);env->DeleteLocalRef(t);}return out;
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(link)(JNIEnv *env,jclass,jint group,jint base,jint pos,jlong time,jint label,jfloat value) {
    BEGIN auto *r=s.get(group);if(!r||!s.valid(*r)||findNumber(group).data)throw std::runtime_error("Activity is already linked");
    for(auto *d:numdatas)if(d->getindex()==base&&!d->receivedbackup()) {
        if(pos<d->getfirstpos()||pos>=d->getlastpos())break;
        const Num n=d->at(pos);
        if(n.time!=time||n.type!=uint32_t(label)||n.value!=value||n.mealptr!=0||!d->valid(&n)||!allowedLabel(label))break;
        d->numchange(&d->at(pos),n.time,n.value,n.type,Num::sportbit|group);
        r->flags=(r->flags|ja::Linked)&~ja::Ambiguous;
        if(backup)backup->wakebackup(wakenums);
        return;
    }
    throw std::runtime_error("Amount changed; reopen the list");
    END
}
