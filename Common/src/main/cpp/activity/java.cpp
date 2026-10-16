/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:52:57 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
#include "amount.hpp"
#include "amount_binding.hpp"
#include "global.hpp"
#include "speed_peak.hpp"
#include "nums/numdata.hpp"
#include "curve/numdisplay.hpp"
#include "curve/numhit.hpp"
#include <jni.h>
#include <memory>
#include <sstream>
#include <iomanip>

extern std::vector<Numdata*> numdatas;
namespace ja=juggluco::activity;
namespace {
ja::Store &store() {
    if(!settings)throw std::runtime_error("Juggluco data is not initialized");
    return ja::globalStore();
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
void migrateAmountBindings(ja::Store &s) {
    if(s.header().amountBindingVersion||numdatas.empty())return;
    // Prefer an unmirrored legacy database over a copied matching baseline.
    for(bool received:{false,true})for(auto *d:numdatas)if(d->receivedbackup()==received)
    for(int p=d->getfirstpos();p<d->getlastpos();++p) {
        const auto &n=d->at(p);
        if(!d->valid(&n)||n.type==settings->data()->bloodvar||!Num::issportref(n.mealptr))continue;
        const auto *r=s.get(ja::indexOf(n.mealptr));
        if(!r||!(r->flags&ja::Linked)||r->amountBinding.count)continue;
        // Preserve legacy local links. If the synchronization flag is set,
        // only an exact saved generated baseline identifies an old local Amount.
        if(!received||((r->flags&ja::Generated)&&n.time==r->generatedTime&&
                n.type==uint32_t(r->generatedLabel)&&n.value==r->generatedValue))
            ja::bindAmount(s,d->getindex(),d->getnewident(),n);
    }
    s.header().amountBindingVersion=1;
}
bool ownAmount(ja::Store &s,Numdata *d,const Num &n) {
    // getindex() logs. Reject ordinary/unbound Amounts before evaluating it.
    if(!Num::issportref(n.mealptr))return false;
    const auto *owner=s.get(ja::indexOf(n.mealptr));
    if(!owner||owner->amountBinding.count!=1)return false;
    return ja::ownsAmount(s,d->getindex(),d->getnewident(),n);
}
void bindAmount(ja::Store &s,Numdata *d,const Num &n,float roundTo=ja::missing) {
    ja::bindAmount(s,d->getindex(),d->getnewident(),n,roundTo);
}
bool unlinkedAmount(ja::Store &s,Numdata *d,const Num &n) {
    // A foreign activity reference may be relinked by time/label just like an
    // ordinary Amount, but its numeric index must never select a local activity.
    return n.mealptr==0||(Num::issportref(n.mealptr)&&!ownAmount(s,d,n));
}
Found findNumber(ja::Store &s,uint32_t group,int metric=-1) {
    group=s.groupOf(group);if(!group)return {};
    std::vector<ja::AmountBinding> bindings;
    for(uint32_t i=1;i<=s.header().count;++i)if(s.groupOf(i)==group) {
        const auto *owner=s.get(i);
        if(metric>=0&&(!(owner->flags&ja::Generated)||owner->generatedMetric!=metric))continue;
        const auto saved=s.view<ja::AmountBinding>(owner->amountBinding);
        if(saved.size()==1&&saved[0].number.mealptr==(Num::sportbit|i)&&
                saved[0].number.type!=settings->data()->bloodvar)bindings.push_back(saved[0]);
    }
    if(bindings.empty())return {};
    // The binding includes the exact Amount, even after a time override. Search
    // its timestamp instead of rescanning all years of Amounts for each group.
    for(auto *d:numdatas) {
        const auto base=d->getindex();const auto database=d->getnewident();
        Found found;
        for(const auto &binding:bindings)if(binding.base==uint32_t(base)&&binding.database==database) {
            const auto &expected=binding.number;
            for(auto *n=d->firstnotless(expected.time);n<d->end()&&n->time==expected.time;++n) {
                if(d->valid(n)&&!memcmp(n,&expected,sizeof(Num))) {
                    const auto pos=d->index(n);
                    if(!found.data||pos<found.pos)found={d,pos};
                    break;
                }
            }
        }
        // Retain the original database/position preference among merged aliases.
        if(found.data)return found;
    }
    return {};
}
void filterActivities(ja::Store &s) {
    std::vector<uint32_t> candidates,durations;
    for(uint32_t i=1;i<=s.header().count;++i) {
        const auto *r=s.get(i);
        if((r->flags&ja::Generated)&&r->generatedMetric==ja::Duration)candidates.push_back(s.groupOf(i));
    }
    std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
    // Old, consolidated aliases retain their flags forever. Only an existing
    // bound duration Amount, not a stale Generated flag, keeps an empty session.
    for(auto group:candidates)if(group&&findNumber(s,group,ja::Duration).data)durations.push_back(group);
    s.filterRecords(durations);
}
void reconcileRemovedAmounts(ja::Store &s) {
    if(numdatas.empty())return;
    std::vector<uint32_t> references;
    if(s.header().removalVersion<1) {
        for(auto *d:numdatas)for(int p=d->getfirstpos();p<d->getlastpos();++p) {
            const auto &n=d->at(p);
            if(d->valid(&n)&&n.type!=settings->data()->bloodvar&&ownAmount(s,d,n))references.push_back(ja::indexOf(n.mealptr));
        }
        s.recoverRemovedAmounts(references);
        LOGGER("Activity: reconciled previously deleted Amounts\n");
    }
    for(auto *d:numdatas)for(int p=d->getfirstpos();p<d->getlastpos();++p) {
        const Num n=d->at(p);
        if(!d->valid(&n)||n.type==settings->data()->bloodvar||!ownAmount(s,d,n))continue;
        const auto action=ja::removedAmount(s,n);
        if(action==ja::RemovedAmount::Keep)continue;
        LOGGER("Activity: %s Amount for removed group=%u base=%d pos=%d\n",
            action==ja::RemovedAmount::Erase?"remove generated":"detach manual",
            s.groupOf(ja::indexOf(n.mealptr)),d->getindex(),p);
        if(action==ja::RemovedAmount::Erase)d->numremove(p,false);
        // numchange(...,0) preserves the old reference. numsavepos explicitly
        // replaces it and sends the usual backup/Kerfstok change notification.
        else d->numsavepos(p,n.time,n.value,n.type,0);
    }
}
void consolidateAmounts(ja::Store &s) {
    std::vector<Found> positions;
    std::vector<Num> numbers;
    for(auto *d:numdatas)for(int p=d->getfirstpos();p<d->getlastpos();++p) {
        const auto &n=d->at(p);
        if(d->valid(&n)&&n.type!=settings->data()->bloodvar&&ownAmount(s,d,n)) {
            positions.push_back({d,p});numbers.push_back(n);
        }
    }
    const auto duplicates=ja::duplicateAmounts(s,numbers);
    for(auto i:duplicates) {
        const auto group=s.groupOf(ja::indexOf(numbers[i].mealptr));
        // Preserve the deletion/association history before removing an Amount.
        // The original source record and its reference stay available forever.
        s.get(group)->flags|=ja::Linked;
        std::atomic_signal_fence(std::memory_order_release);
        LOGGER("Activity: remove duplicate Amount group=%u reference=%u base=%d pos=%d\n",
               group,ja::indexOf(numbers[i].mealptr),positions[i].data->getindex(),positions[i].pos);
        // The positional overload leaves a tombstone and does not shift entries.
        positions[i].data->numremove(positions[i].pos,false);
    }
}
std::vector<ja::AmountMatchTarget> amountMatchTargets(ja::Store &s) {
    std::vector<ja::AmountMatchTarget> targets;
    for(uint32_t i=1;i<=s.header().count;++i)if(s.get(i)->group==i&&s.groupVisible(i)) {
        const auto *r=s.get(s.representative(i));if(!r)continue;
        auto found=findNumber(s,i);
        int label=found.data?int(found.data->at(found.pos).type):s.rule(r->type).label;
        if(!allowedLabel(label))label=-1;
        const auto times=s.interval(i);targets.push_back({i,label,times[0],times[1]});
    }
    return targets;
}
struct AmountCandidate {Found found;int count=0;bool ambiguous=false;};
AmountCandidate amountCandidate(ja::Store &s,uint32_t group,int label,
                                std::span<const ja::AmountMatchTarget> targets) {
    std::vector<ja::AmountMatchTarget> local;
    if(targets.empty()){local=amountMatchTargets(s);targets=local;}
    const auto times=s.interval(group),range=ja::amountMatchRange(times[0],times[1]);
    AmountCandidate result;
    for(auto *d:numdatas)for(auto *n=d->firstnotless(range[0]);n<d->end()&&n->time<=range[1];++n) {
        if(!d->valid(n)||n->type!=uint32_t(label)||!unlinkedAmount(s,d,*n))continue;
        const auto preferred=ja::preferredAmountGroup(targets,label,s.header().defaultLabel,n->time);
        if(preferred==group){result.found={d,d->index(n)};++result.count;}
        else if(!preferred)result.ambiguous=true;
    }
    result.ambiguous|=result.count>1;
    return result;
}
void linkExistingAmount(ja::Store &s,uint32_t group,Found candidate,Found generated={}) {
    const Num n=candidate.data->at(candidate.pos);
    // Keep an alias reference when replacing a generated duplicate. The original
    // manual time, label and value remain unchanged, including later corrections.
    const uint32_t reference=generated.data?generated.data->at(generated.pos).mealptr:Num::sportbit|group;
    const Num linked{.time=n.time,.mealptr=reference,.value=n.value,.type=n.type};
    bindAmount(s,candidate.data,linked);
    candidate.data->numchange(&candidate.data->at(candidate.pos),n.time,n.value,n.type,reference);
    auto *owner=s.get(ja::indexOf(reference));
    owner->flags=(owner->flags|ja::Linked)&~(ja::Generated|ja::LabelOverride|ja::ValueOverride|ja::TimeOverride);
    s.get(group)->flags=(s.get(group)->flags|ja::Linked)&~ja::Ambiguous;
    s.changed();
    if(generated.data) {
        LOGGER("Activity: replace generated duplicate with existing Amount group=%u base=%d pos=%d\n",
            group,candidate.data->getindex(),candidate.pos);
        // A tombstone does not shift the surviving Amount or delete its activity.
        generated.data->numremove(generated.pos,false);
    }
}
void associate(ja::Store &s,uint32_t group,bool applyMapping=false, std::span<const ja::AmountMatchTarget> targets={}) {
    if(settings->data()->nochangenum)
        return;
    // Retrieval and resume must not create Amounts or labels without consent.
    // An explicitly requested mapping change may still edit existing Amounts.
    if(!applyMapping&&!(s.header().options&ja::CreateAmounts))return;
    auto *root=s.get(group); if(!root||!s.groupVisible(group)) return;
    const auto *representative=s.get(s.representative(group));if(!representative)return;
    auto rule=s.rule(representative->type);
    if(!allowedLabel(rule.label)) rule.label=defaultLabel(s);
    auto found=findNumber(s,group);
    if(applyMapping&&!found.data) return;
    // Baselines belong to the actual reference on the surviving Amount, which
    // can be an alias after a merge. Do not mistake another source's value for
    // a manual override or rewrite its opaque mealptr.
    auto *owner=found.data?s.get(ja::indexOf(found.data->at(found.pos).mealptr)):root;
    const int metric=ja::amountMetric(*owner,rule,applyMapping);
    const auto *measurement=ja::measurementFor(s,group,metric);
    if(!measurement) return;
    const auto summary=ja::groupMeasurement(s,group,*measurement);
    const auto times=s.interval(group);
    const float value=ja::amountValue(summary,rule.metric,rule.roundTo);
    if(found.data) {
        auto n=found.data->at(found.pos);
        root->flags|=ja::Linked;
        owner->flags|=ja::Linked;
        if(!applyMapping&&ja::untouchedGenerated(*owner,n)) {
            const auto candidate=amountCandidate(s,group,n.type,targets);
            if(candidate.ambiguous)root->flags|=ja::Ambiguous;
            else if(candidate.count==1) {
                linkExistingAmount(s,group,candidate.found,found);
                return;
            }
        }
        if(owner->flags&ja::Generated) {
            const auto update=ja::updateAmount(*owner,n,summary,rule,applyMapping,times[0],ja::amountRounding(s,*owner));
            const auto &next=update.number;
            if(next.type!=n.type||next.value!=n.value||next.time!=n.time) {
                const auto origin=s.text(measurement->origin);
                LOGGER("Activity Amount: group=%u metric=%d origin=%.*s value=%g (was %g)\n",
                    group,update.metric,int(origin.size()),origin.data(),double(next.value),double(n.value));
                found.data->numchange(&found.data->at(found.pos),next.time,next.value,next.type,next.mealptr);
            }
            owner->generatedLabel=next.type;owner->generatedValue=next.value;owner->generatedTime=next.time;
            // Remember the unit even when two different measures happen to
            // produce the same number, so later source corrections use it.
            owner->generatedMetric=update.metric;
            bindAmount(s,found.data,next,update.roundTo);
        }
        return;
    }
    // The Linked flag records a past association. A mirror can replace or
    // truncate Amounts without changing the cached activities. Reconcile those
    // entries again; explicit local deletion is protected by groupVisible().
    const auto candidate=amountCandidate(s,group,rule.label,targets);
    if(candidate.ambiguous) {root->flags|=ja::Ambiguous;return;}
    root->flags&=~ja::Ambiguous;
    if(candidate.count==1) {
        linkExistingAmount(s,group,candidate.found);
        return;
    }
    if(!allowedLabel(rule.label)||!std::isfinite(value)) return;
    for(auto *d:numdatas) if(d->getindex()==1) {
        const auto origin=s.text(measurement->origin);
        LOGGER("Activity Amount: group=%u metric=%d origin=%.*s value=%g\n",
            group,rule.metric,int(origin.size()),origin.data(),double(value));
        // Record ownership before insertion, so replay recognizes its origin.
        root->generatedLabel=rule.label; root->generatedMetric=rule.metric;
        root->generatedValue=value; root->generatedTime=times[0];
        root->flags=(root->flags|ja::Generated)&~(ja::LabelOverride|ja::ValueOverride|ja::TimeOverride);
        bindAmount(s,d,Num{.time=times[0],.mealptr=Num::sportbit|group,.value=value,.type=uint32_t(rule.label)},rule.roundTo);
        d->numsave(times[0],value,rule.label,Num::sportbit|group);
        if(findNumber(s,group).data) root->flags|=ja::Linked;
        break;
    }
}
}

#define AJ(name) Java_tk_glucodata_ActivityNative_##name
#define BEGIN try { auto &s=store(); std::lock_guard guard(s.mutex);
#define IMPORT BEGIN if(settings->staticnum())throw std::runtime_error("Health Connect retrieval is disabled while receiving mirrored Amounts");
#define END } catch(const std::exception &e) { error(env,e); }
extern "C" JNIEXPORT jlongArray JNICALL AJ(config)(JNIEnv *env,jclass) {
    BEGIN
    auto &h=s.header(); jlong v[]={h.from,h.interval,h.lastSuccess,h.defaultLabel,h.generation,jlong(h.options)};
    auto out=env->NewLongArray(6); env->SetLongArrayRegion(out,0,6,v); return out;
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(configure)(JNIEnv *env,jclass,jlong from,jint interval,jint label) {
    BEGIN
    if(from<0||from>UINT32_MAX||interval<0||(interval>0&&interval<900)) throw std::invalid_argument("Invalid retrieval settings");
    if(label>=0&&!allowedLabel(label)) throw std::invalid_argument("Choose an activity label");
    if(s.header().from!=uint32_t(from)) {s.resetTokens();++s.header().generation;s.header().lastSuccess=0;}
    s.header().from=from;s.header().interval=interval;s.header().defaultLabel=label;
    s.changed();
    END
}
extern "C" JNIEXPORT void JNICALL AJ(setOptions)(JNIEnv *env,jclass,jint options) {
    BEGIN
    if(options<0||(uint64_t(options)&~(ja::CreateAmounts|ja::UseMiles)))throw std::invalid_argument("Invalid activity options");
    s.header().options=options;
    filterActivities(s);s.changed();
    END
}
extern "C" JNIEXPORT jstring JNICALL AJ(token)(JNIEnv *env,jclass,jint type) {
    BEGIN
    if(type<0||type>=16) throw std::out_of_range("Token type");
    return env->NewStringUTF(std::string(s.token(type)).c_str());
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(setToken)(JNIEnv *env,jclass,jint type,jstring token) {
    IMPORT Text t(env,token);s.token(type,t); END
}
extern "C" JNIEXPORT jstring JNICALL AJ(pendingToken)(JNIEnv *env,jclass,jint type) {
    BEGIN return env->NewStringUTF(std::string(s.pendingToken(type)).c_str());
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(checkpoint)(JNIEnv *env,jclass,jint type,jstring token,jboolean dirty) {
    IMPORT Text t(env,token);s.checkpoint(type,t,dirty); END
}
extern "C" JNIEXPORT jint JNICALL AJ(markDetails)(JNIEnv *env,jclass,jint type,jlong from,jlong to,jstring origin) {
    IMPORT Text app(env,origin);return s.markDetails(type,from,to,app); END return 0;
}
extern "C" JNIEXPORT jint JNICALL AJ(markAllDetails)(JNIEnv *env,jclass) {
    IMPORT return s.markAllDetails(); END return 0;
}
extern "C" JNIEXPORT void JNICALL AJ(resetTokens)(JNIEnv *env,jclass) {IMPORT s.resetTokens();++s.header().generation;s.header().lastSuccess=0; END}
extern "C" JNIEXPORT void JNICALL AJ(beginSessionSnapshot)(JNIEnv *env,jclass) {IMPORT s.beginSessionSnapshot(); END}
extern "C" JNIEXPORT void JNICALL AJ(permissions)(JNIEnv *env,jclass,jint mask) {
    IMPORT if(s.header().readPermissions!=uint32_t(mask)) {
        s.resetTokens();++s.header().generation;s.header().lastSuccess=0;s.header().readPermissions=mask;
    } END
}
extern "C" JNIEXPORT void JNICALL AJ(succeeded)(JNIEnv *env,jclass,jlong now) {IMPORT s.header().lastSuccess=now;s.changed(); END}
extern "C" JNIEXPORT jint JNICALL AJ(source)(JNIEnv *env,jclass,jstring origin,jstring device) {
    IMPORT Text a(env,origin),b(env,device);return s.source(a,b); END return 0;
}
extern "C" JNIEXPORT jstring JNICALL AJ(sourceName)(JNIEnv *env,jclass,jint source) {
    BEGIN auto item=s.source(source);std::string name(s.text(item.origin));
    if(auto device=s.text(item.device);!device.empty()){name+=" · ";name+=device;}
    return env->NewStringUTF(name.c_str()); END return nullptr;
}
extern "C" JNIEXPORT jint JNICALL AJ(upsert)(JNIEnv *env,jclass,jstring id,jstring origin,jstring title,jstring notes,jlong start,jlong end,jint type,jlong modified,jint source) {
    IMPORT
    if(start<=0||end>UINT32_MAX||end<=start||type<0) throw std::invalid_argument("Invalid activity interval");
    Text a(env,id),b(env,origin),c(env,title),d(env,notes);
    ja::Record r{};r.start=start;r.end=end;r.type=type;r.modified=modified;r.source=source;
    const auto index=s.upsert(a,b,c,d,r);const auto originText=std::string_view(b);
    LOGGER("Activity: session=%u group=%u type=%u start=%u end=%u origin=%.*s\n",
           index,s.groupOf(index),r.type,r.start,r.end,int(originText.size()),originText.data());
    return index;
    END return 0;
}
extern "C" JNIEXPORT void JNICALL AJ(deleted)(JNIEnv *env,jclass,jstring id) {
    IMPORT Text a(env,id);
    for(uint32_t i=1;i<=s.header().count;++i) if(s.text(s.get(i)->id)==std::string_view(a)) {
        s.get(i)->flags|=ja::Deleted;
        s.changed();
        LOGGER("Activity: Health Connect deleted source=%u group=%u\n",i,s.groupOf(i));
    }
    END
}
extern "C" JNIEXPORT void JNICALL AJ(remove)(JNIEnv *env,jclass,jint group) {
    BEGIN
    if(!s.removeGroup(group))throw std::runtime_error("Activity is unavailable");
    LOGGER("Activity: user deleted group=%u\n",s.groupOf(group));
    if(backup)backup->wakebackup(wakenums);
    // Number cleanup waits until any open Amount editor has closed.
    END
}
extern "C" JNIEXPORT void JNICALL AJ(removeSource)(JNIEnv *env,jclass,jint group,jint source) {
    BEGIN
    if(!s.removeSource(group,source))throw std::runtime_error("Activity source is unavailable");
    filterActivities(s);
    if(backup)backup->wakebackup(wakenums);
    END
}
extern "C" JNIEXPORT jintArray JNICALL AJ(sourceIDs)(JNIEnv *env,jclass,jint group) {
    BEGIN std::vector<jint> ids;group=s.groupOf(group);
    auto add=[&](uint32_t id){if(id&&!s.sourceRemoved(group,id)&&std::find(ids.begin(),ids.end(),id)==ids.end())ids.push_back(id);};
    for(uint32_t i=1;i<=s.header().count;++i)if(s.groupOf(i)==uint32_t(group)&&s.sourceVisible(i)) {
        const auto &r=*s.get(i);add(r.source);
        for(const auto &heart:s.heartSummaries(r))add(heart.source);
    }
    auto out=env->NewIntArray(ids.size());if(!ids.empty())env->SetIntArrayRegion(out,0,ids.size(),ids.data());return out;
    END return nullptr;
}
extern "C" JNIEXPORT jintArray JNICALL AJ(indices)(JNIEnv *env,jclass,jboolean pending) {
    BEGIN std::vector<jint> list;
    if(!pending){s.mergeOverlaps();filterActivities(s);}
    for(uint32_t i=1;i<=s.header().count;++i) {auto &r=*s.get(i);
        if(pending?(s.sourceVisible(i)&&!(r.flags&ja::NotReturned)&&(r.flags&ja::NeedsDetails)&&r.start>=s.header().from):
            (r.group==i&&s.groupVisible(i)))list.push_back(i);
    }
    std::sort(list.begin(),list.end(),[&](int a,int b){return s.get(a)->start>s.get(b)->start;});
    auto out=env->NewIntArray(list.size());if(!list.empty())env->SetIntArrayRegion(out,0,list.size(),list.data());return out;
    END return nullptr;
}
extern "C" JNIEXPORT jint JNICALL AJ(representative)(JNIEnv *env,jclass,jint group) {
    BEGIN return s.representative(group); END return 0;
}
extern "C" JNIEXPORT jdoubleArray JNICALL AJ(record)(JNIEnv *env,jclass,jint index) {
    BEGIN auto *r=s.get(index);if(!r||!s.valid(*r))return nullptr;
    jdouble v[21]={double(r->group),double(r->flags),double(r->type),double(r->start),double(r->end),double(r->modified)};
    for(size_t i=0;i<ja::FieldCount;++i)v[6+i]=r->values[i];
    v[18]=r->generatedMetric;v[19]=r->generatedValue;
    v[20]=r->maximumHR>0?r->maximumHR:ja::missing;
    auto out=env->NewDoubleArray(21);env->SetDoubleArrayRegion(out,0,21,v);return out;
    END return nullptr;
}
extern "C" JNIEXPORT jdoubleArray JNICALL AJ(peaks)(JNIEnv *env,jclass,jint index) {
    BEGIN auto *r=s.get(index);if(!r||!s.valid(*r))return nullptr;
    // Scan cached samples only when opening details, not during import/listing.
    const auto speed=ja::filteredSpeedPeak(s.measureSamples(*r,s.view<ja::Sample>(r->speed)),
        r->type,int64_t(r->start)*1000,int64_t(r->end)*1000);
    const auto heart=s.heartPeak(*r);
    jdouble v[]={speed.value,double(speed.time),double(heart.time)};
    auto out=env->NewDoubleArray(3);env->SetDoubleArrayRegion(out,0,3,v);return out;
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
    group=s.groupOf(group);
    for(uint32_t i=1;i<=s.header().count;++i)if(s.groupOf(i)==uint32_t(group)&&s.sourceVisible(i))list.push_back(i);
    auto out=env->NewIntArray(list.size());if(!list.empty())env->SetIntArrayRegion(out,0,list.size(),list.data());return out;
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(metrics)(JNIEnv *env,jclass,jint index,jdoubleArray values) {
    IMPORT auto *r=s.get(index);if(!r||env->GetArrayLength(values)!=ja::FieldCount)throw std::invalid_argument("Activity metrics");
    jdouble v[ja::FieldCount];env->GetDoubleArrayRegion(values,0,ja::FieldCount,v);
    for(size_t i=0;i<ja::FieldCount;++i)r->values[i]=v[i];
    s.changed();
    END
}
extern "C" JNIEXPORT void JNICALL AJ(samples)(JNIEnv *env,jclass,jint index,jint kind,jlongArray times,jfloatArray values,jintArray sources) {
    IMPORT
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
    if(r)for(const auto &heart:s.heartSummaries(*r))
        values.insert(values.end(),{double(heart.source),heart.statistics[0],heart.statistics[1],heart.statistics[2],heart.resting,heart.statistics[3],double(heart.maximumTime)});
    auto out=env->NewDoubleArray(values.size());if(!values.empty())env->SetDoubleArrayRegion(out,0,values.size(),values.data());return out;
    END return nullptr;
}
extern "C" JNIEXPORT jint JNICALL AJ(heartSource)(JNIEnv *env,jclass,jint index) {
    BEGIN auto *r=s.get(index);if(!r)return 0;
    const auto selected=s.heartSamples(*r);return selected.empty()?0:selected.front().source;
    END return 0;
}
extern "C" JNIEXPORT void JNICALL AJ(complete)(JNIEnv *env,jclass,jint index) {
    IMPORT auto *r=s.get(index);if(r){r->flags&=~ja::NeedsDetails;s.changed();} END
}
extern "C" JNIEXPORT void JNICALL AJ(linkAmounts)(JNIEnv *env,jclass) {
    BEGIN
    migrateAmountBindings(s);
    // Also repair cached records imported with the former strict matcher, even
    // when Health Connect has no new changes. Safe here: no Amount editor is open.
    if(auto merged=s.mergeOverlaps())LOGGER("Activity: merged %u cached groups\n",merged);
    filterActivities(s);
    // This is the current user setting. receivedbackup is a historical Garmin
    // synchronization flag and is not cleared by disabling Mirror sends amounts.
    if(settings->data()->nochangenum) {
        LOGGER("Activity: saved details retained; Amount linking disabled by Mirror sends amounts\n");
        return;
    }
    // Deletion does not require consent to create new Amounts.
    reconcileRemovedAmounts(s);
    if(s.header().options&ja::CreateAmounts)consolidateAmounts(s);
    const auto targets=(s.header().options&ja::CreateAmounts)?amountMatchTargets(s):std::vector<ja::AmountMatchTarget>{};
    unsigned visible=0,linked=0;
    for(uint32_t i=1;i<=s.header().count;++i)if(s.get(i)->group==i) {
        associate(s,i,false,targets);
        if(s.groupVisible(i))++visible;
    }
    // Count in one pass instead of rescanning the Amount database per group.
    std::vector<bool> counted(s.header().count+1,false);
    for(auto *d:numdatas)for(int p=d->getfirstpos();p<d->getlastpos();++p) {
        const auto &n=d->at(p);
        if(!d->valid(&n)||n.type==settings->data()->bloodvar||!ownAmount(s,d,n))continue;
        const auto group=s.groupOf(ja::indexOf(n.mealptr));
        if(group&&!counted[group]&&s.groupVisible(group)){counted[group]=true;++linked;}
    }
    LOGGER("Activity: saved groups=%u, linked Amounts=%u, createAmounts=%d\n",visible,linked,bool(s.header().options&ja::CreateAmounts));
    if(backup)backup->wakebackup(wakenums);
    END
}
extern "C" JNIEXPORT jintArray JNICALL AJ(rule)(JNIEnv *env,jclass,jint type) {
    BEGIN auto r=s.rule(type);jint v[]={r.label,r.metric};auto out=env->NewIntArray(2);env->SetIntArrayRegion(out,0,2,v);return out; END return nullptr;
}
extern "C" JNIEXPORT jfloat JNICALL AJ(roundTo)(JNIEnv *env,jclass,jint type) {
    BEGIN const auto r=s.rule(type);return ja::roundingIncrement(r.metric,r.roundTo); END return 0.1f;
}
extern "C" JNIEXPORT void JNICALL AJ(setRule)(JNIEnv *env,jclass,jint type,jint label,jint metric,jfloat roundTo,jboolean existing) {
    BEGIN if(!allowedLabel(label))throw std::invalid_argument("Choose an activity label");
    migrateAmountBindings(s);
    if(!std::isfinite(roundTo)||roundTo<=0)throw std::invalid_argument("Enter a positive rounding increment");
    s.setRule({type,label,metric,roundTo});
    if(existing) {
        for(uint32_t i=1;i<=s.header().count;++i) {
                if(s.get(i)->group==i) {
                    auto *r=s.get(s.representative(i));
                    if(r&&r->type==uint32_t(type))
                        associate(s,i,true);
                    }
             }
        }
    if(backup)
        backup->wakebackup(wakenums);
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
    migrateAmountBindings(s);
    if(!n||n->type==settings->data()->bloodvar||!Num::issportref(n->mealptr))return 0;
    if(!ownAmount(s,numhit->numdisplay,*n))return 0;
    return s.groupOf(ja::indexOf(n->mealptr));
    END return 0;
}
extern "C" JNIEXPORT jobjectArray JNICALL AJ(candidates)(JNIEnv *env,jclass,jint group) {
    BEGIN std::vector<std::string> list;group=s.groupOf(group);if(!s.groupVisible(group))throw std::runtime_error("Activity is unavailable");
    migrateAmountBindings(s);
    const auto times=s.interval(group),range=ja::amountMatchRange(times[0],times[1]);
    for(auto *d:numdatas)for(auto *n=d->firstnotless(range[0]);n<d->end()&&n->time<=range[1];++n)
        if(d->valid(n)&&unlinkedAmount(s,d,*n)&&allowedLabel(n->type)) {
            std::ostringstream text;text<<std::setprecision(std::numeric_limits<float>::max_digits10)<<d->getindex()<<':'<<d->index(n)<<':'<<n->time<<':'<<n->type<<':'<<n->value;
            list.push_back(text.str());
        }
    auto out=env->NewObjectArray(list.size(),env->FindClass("java/lang/String"),nullptr);
    for(size_t i=0;i<list.size();++i){auto t=env->NewStringUTF(list[i].c_str());env->SetObjectArrayElement(out,i,t);env->DeleteLocalRef(t);}return out;
    END return nullptr;
}
extern "C" JNIEXPORT void JNICALL AJ(link)(JNIEnv *env,jclass,jint group,jint base,jint pos,jlong time,jint label,jfloat value) {
    BEGIN group=s.groupOf(group);auto *r=s.get(group);if(!s.groupVisible(group))throw std::runtime_error("Activity is unavailable");
    if(settings->data()->nochangenum)throw std::runtime_error("Mirror sends amounts is enabled");
    migrateAmountBindings(s);
    if(findNumber(s,group).data)throw std::runtime_error("Activity is already linked");
    for(auto *d:numdatas)if(d->getindex()==base) {
        if(pos<d->getfirstpos()||pos>=d->getlastpos())break;
        const Num n=d->at(pos);
        if(n.time!=time||n.type!=uint32_t(label)||n.value!=value||!unlinkedAmount(s,d,n)||!d->valid(&n)||!allowedLabel(label))break;
        linkExistingAmount(s,group,{d,pos});
        if(backup)backup->wakebackup(wakenums);
        return;
    }
    throw std::runtime_error("Amount changed; reopen the list");
    END
}
