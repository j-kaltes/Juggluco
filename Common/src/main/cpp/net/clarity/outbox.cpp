// SPDX-License-Identifier: GPL-3.0-or-later
#include "outbox.hpp"
#include "mapped.hpp"
#include "mapped_index.hpp"
#include "protocol.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <tuple>
namespace clarity {
namespace {
struct alignas(8) Root {
    char account[40], installation[40];
    int32_t sequence;
    uint32_t reserved;
    uint64_t end, times, events, sessions, sources, pending;
    int64_t count, first, last;
    char receipt[40];
    uint64_t timeIndex, eventIndex, sessionIndex, sourceIndex, timelineIndex;
    uint32_t redoSize, redoChecksum;
};
struct alignas(8) SensorNode {
    uint64_t previous;
    MappedString sensor;
    int64_t start, end, first;
};
struct alignas(8) SessionNode { uint64_t previous; MappedString sensor; };
struct alignas(8) TimeNode { uint64_t previous; MappedString values; };
struct alignas(8) EventNode {
    uint64_t previous, revision;
    MappedString key, value, description, displayTime;
    char id[40];
    int64_t time;
    uint32_t kind;
    int32_t firstPosition;
    uint32_t deleted, updated;
};
struct alignas(8) PendingNode {
    char id[40];
    uint64_t times, events, sessions;
    int64_t count, first, last;
    uint32_t wireSize, wireChecksum;
};
static_assert(sizeof(Root)==248 && sizeof(SensorNode)==48 && sizeof(SessionNode)==24);
static_assert(sizeof(EventNode)==144 && sizeof(PendingNode)==96 && sizeof(TimeNode)==24);
void uuidField(char (&field)[40], std::string_view value) {
    if (!isUuid(value)) throw Error("Invalid Clarity mapped identity");
    memset(field,0,sizeof field);
    memcpy(field,value.data(),value.size());
}
std::string uuidString(const char (&field)[40]) {
    if (field[36] || !isUuid(std::string_view(field,36)))
        throw Error("Invalid saved Clarity mapped identity");
    return {field,36};
}
Root initial(const std::string &account,const std::string &installation) {
    Root r{};
    uuidField(r.account,account); uuidField(r.installation,installation);
    r.end=8;
    r.sequence=static_cast<int32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    return r;
}
GlucoseRange range(const std::vector<int64_t> &times) {
    GlucoseRange r{};
    for (auto t:times) { ++r.count; if (!r.first || t<r.first) r.first=t; r.last=std::max(r.last,t); }
    return r;
}
std::string quoted(std::string value) {
    if (value.size()>160) value=value.substr(0,160)+"...";
    return Json(value).dump(-1,' ',false,Json::error_handler_t::replace);
}
void logEvent(const char *phase,const std::string &batch,const char *action,const SavedEvent &e) {
    const auto r=eventRecord(e.content,0,action); // Log escaping only; not persisted.
    diagnostic("%s event: batch=%s action=%s id=%s key=%s type=%s subtype=%s time=%s display=%s value=%s units=%s",
        phase,batch.c_str(),action,e.content.id.c_str(),e.key.c_str(),
        quoted(r.value("Name","")).c_str(),quoted(r.value("SubType","")).c_str(),
        quoted(timestamp(e.content.time)).c_str(),quoted(e.content.displayTime).c_str(),
        quoted(e.content.value).c_str(),quoted(r.value("Units","")).c_str());
}
void logGlucose(const char *phase,GlucoseRange r) {
    if (!r.count) diagnostic("%s: count=0",phase);
    else diagnostic("%s: count=%lld first=%s last=%s",phase,(long long)r.count,
                    timestamp(r.first).c_str(),timestamp(r.last).c_str());
}
}
struct Outbox::Impl {
    MappedCheckpoint<Root> state;
    MappedArena data;
    MappedFile wire, redo;
    std::string account, installation;
    bool failed=false;
    Impl(const std::string &path,const std::string &a,const std::string &i)
        :state(path,"G7OUTB02",initial(a,i)),data(path+".data"),wire(path+".request"),redo(path+".redo"),
         account(uuidString(state.get().account)),installation(uuidString(state.get().installation)) {
        if (a!=account || i!=installation) throw Error("Clarity outbox identity mismatch");
        data.checkEnd(state.get().end);
        recover();
        const auto &r = state.get();
        const MappedIndexView index(data, r.end);
        for (auto root : {r.timeIndex, r.eventIndex, r.sessionIndex, r.sourceIndex, r.timelineIndex})
            index.checkRoot(root);
        if (r.count < 0 || bool(r.count) != bool(r.timeIndex) ||
            (r.count && (r.first <= 0 || r.last < r.first)))
            throw Error("Invalid mapped Clarity glucose progress");
        if (r.pending) (void)pendingBody();
        diagnostic("mapped outbox reopened directly: glucose=%lld pending=%d; no index reconstruction",
                   (long long)r.count, bool(r.pending));
    }
    void recover() {
        auto r = state.get();
        if (!r.redoSize) return;
        if (r.redoSize % sizeof(IndexWrite) || r.redoSize > 16 * 1024 * 1024 ||
            r.redoSize > redo.size() || r.redoChecksum != mappedChecksum(redo.data(), r.redoSize))
            throw Error("Clarity mapped index transaction is damaged");
        const auto writes = std::span(reinterpret_cast<const IndexWrite *>(redo.data()),
                                      r.redoSize / sizeof(IndexWrite));
        uint64_t last = 0;
        for (const auto &w : writes) {
            if (w.offset <= last || w.offset % alignof(IndexNode) ||
                !w.node.height || w.node.height > 64)
                throw Error("Invalid Clarity mapped index transaction");
            (void)data.at(w.offset, sizeof(IndexNode), r.end);
            last = w.offset;
        }
        std::vector<std::pair<size_t, size_t>> pages;
        pages.reserve(writes.size());
        for (const auto &w : writes) {
            data.write(w.offset, w.node, r.end);
            pages.emplace_back(w.offset, sizeof(IndexNode));
        }
        data.syncRanges(pages);
        r.redoSize = r.redoChecksum = 0;
        state.save(r);
    }
    void healthy() const { if (failed) throw Error("Clarity mapped outbox must be reopened after a write failure"); }
    template<class T,class F> void walk(uint64_t head,uint64_t stop,F action) const {
        while (head!=stop) {
            if (!head) throw Error("Invalid Clarity mapped record chain");
            const auto node=data.get<T>(head,state.get().end); // Callback may extend/remap data.
            if (node.previous>=head) throw Error("Invalid Clarity mapped record order");
            action(node,head); head=node.previous;
        }
    }
    std::string string(MappedString s) const { return std::string(data.get(s,state.get().end)); }
    std::span<const int64_t> times(const TimeNode &n) const {
        if (n.values.length%sizeof(int64_t) || n.values.offset%alignof(int64_t))
            throw Error("Invalid mapped Clarity glucose times");
        return {static_cast<const int64_t*>(data.at(n.values.offset,n.values.length,state.get().end)),
                n.values.length/sizeof(int64_t)};
    }
    SavedEvent event(const EventNode &n) const {
        if (n.kind<uint32_t(NumberKind::Rapid) || n.kind>uint32_t(NumberKind::Blood) || n.deleted>1 || n.updated>1)
            throw Error("Invalid mapped Clarity event");
        return {string(n.key),{uuidString(n.id),string(n.displayTime),string(n.value),string(n.description),
                n.time,NumberKind(n.kind)},n.revision,n.firstPosition,bool(n.deleted),bool(n.updated)};
    }
    SavedEvent event(uint64_t pos) const { return event(data.get<EventNode>(pos,state.get().end)); }
    uint64_t putEvent(Root &r,const SavedEvent &e,uint64_t previous) {
        EventNode n{}; n.previous=previous; n.revision=e.revision;
        n.key=data.put(r.end,std::string_view(e.key));
        n.value=data.put(r.end,std::string_view(e.content.value));
        n.description=data.put(r.end,std::string_view(e.content.description));
        n.displayTime=data.put(r.end,std::string_view(e.content.displayTime));
        uuidField(n.id,e.content.id); n.time=e.content.time; n.kind=uint32_t(e.content.kind);
        n.firstPosition=e.firstPosition; n.deleted=e.deleted; n.updated=e.updated;
        return data.put(r.end,n);
    }
    void commit(Root next, const MappedIndexEdit *edit = nullptr) {
        healthy();
        try {
            // The redo buffer contains only existing nodes modified in this
            // transaction. It is reusable; it does not grow with the history.
            if (edit && !edit->writes().empty()) {
                const auto size = edit->writes().size() * sizeof(IndexWrite);
                if (size > 16 * 1024 * 1024) throw Error("Clarity index transaction exceeds size limit");
                redo.ensure(size);
                size_t offset = 0;
                for (const auto &[position, node] : edit->writes()) {
                    IndexWrite w{position, node};
                    memcpy(redo.data() + offset, &w, sizeof w);
                    offset += sizeof w;
                }
                next.redoSize = size;
                next.redoChecksum = mappedChecksum(redo.data(), size);
                redo.sync(0, size);
            }
            // New nodes/records first; then a durable root with the redo intent;
            // then in-place existing-node writes. Recovery repeats them safely.
            data.sync(state.get().end, next.end);
            state.save(next);
            recover();
        } catch (...) { failed = true; throw; }
    }
    std::string_view pendingBody() const {
        const auto &r = state.get();
        if (!r.pending) return {};
        const auto &p = data.get<PendingNode>(r.pending, r.end);
        if (!p.wireSize || p.wireSize > wire.size() ||
            p.wireChecksum != mappedChecksum(wire.data(), p.wireSize))
            throw Error("Clarity pending request is truncated or damaged; upload stopped");
        return {reinterpret_cast<const char *>(wire.data()), p.wireSize};
    }
    PendingBatch pending() const {
        const auto &r=state.get();
        if (!r.pending) return {};
        const auto &p=data.get<PendingNode>(r.pending,r.end);
        PendingBatch out;
        out.id=uuidString(p.id);
        out.body=pendingBody();
        walk<TimeNode>(p.times,r.times,[&](const TimeNode &n,uint64_t) {
            auto v=times(n); out.times.insert(out.times.end(),v.begin(),v.end());
        });
        walk<SessionNode>(p.sessions,r.sessions,[&](const SessionNode &n,uint64_t) {out.sessions.push_back(string(n.sensor));});
        walk<EventNode>(p.events,r.events,[&](const EventNode &n,uint64_t) {out.events.push_back(event(n));});
        // Linked nodes were appended in source order. Restore that order only;
        // neither glucose nor amount inputs are sorted here.
        std::reverse(out.sessions.begin(),out.sessions.end());
        std::reverse(out.events.begin(),out.events.end());
        return out;
    }
    void log(const char *phase,const PendingBatch &p,
             const std::vector<SavedEvent> *previousValues = nullptr) const {
        size_t created=0,updated=0,deleted=0,comments=0;
        for (const auto &e:p.events) {
            if (e.deleted) ++deleted; else if(e.updated) ++updated; else ++created;
            if (e.content.kind==NumberKind::Note) ++comments;
        }
        diagnostic("%s batch: id=%s events=%zu new=%zu updated=%zu deleted=%zu comments=%zu",
            phase,p.id.c_str(),p.events.size(),created,updated,deleted,comments);
        const auto &r = state.get();
        MappedIndexView index(data, r.end);
        for (const auto &e:p.events) {
            if (e.updated && !e.deleted) {
                if (previousValues) {
                    auto it = std::find_if(previousValues->begin(), previousValues->end(),
                                           [&](const auto &old) { return old.key == e.key; });
                    if (it != previousValues->end()) logEvent(phase, p.id, "Previous", *it);
                } else if (auto old = index.findString(r.eventIndex, e.key))
                    logEvent(phase, p.id, "Previous", event(old));
            }
            logEvent(phase,p.id,e.deleted?"Deleted":e.updated?"Updated":"New",e);
        }
    }
    bool newer(uint64_t a, uint64_t b, uint64_t end) const {
        if (!b) return true;
        const auto &one = data.get<SensorNode>(a, end);
        const auto &two = data.get<SensorNode>(b, end);
        return std::tuple(one.end, one.start, data.get(one.sensor, end)) >
               std::tuple(two.end, two.start, data.get(two.sensor, end));
    }
    bool prepare(const Snapshot &s,int64_t since,int64_t now,bool sendNumbers) {
        healthy();
        Root r=state.get();
        // Remember source activation even while an old request is awaiting its
        // acknowledgment. This does not change any of that request's content.
        MappedIndexEdit discovery(data, r.end);
        auto remember = [&](const SensorSource &source) {
            if (source.first > now || source.first < source.start ||
                discovery.findString(r.sourceIndex, source.sensor)) return;
            SensorNode node{};
            node.previous = r.sources;
            node.sensor = data.put(r.end, std::string_view(source.sensor));
            node.start = source.start; node.end = source.end; node.first = source.first;
            r.sources = data.put(r.end, node);
            r.sourceIndex = discovery.putString(r.sourceIndex, node.sensor, r.sources);
            // Persist the same monotonic wear-end selection timeline that was
            // previously reconstructed in selectReadings on every scan.
            const auto prior = discovery.floor(r.timelineIndex, source.first);
            if (newer(r.sources, prior ? prior->value : 0, r.end)) {
                r.timelineIndex = discovery.putNumber(r.timelineIndex, source.first, r.sources);
                for (;;) {
                    const auto later = discovery.ceiling(r.timelineIndex, source.first, false);
                    if (!later || newer(later->value, r.sources, r.end)) break;
                    r.timelineIndex = discovery.eraseNumber(r.timelineIndex, later->number);
                }
            }
            diagnostic("sensor source remembered: end=%lld first=%lld start=%lld",
                (long long)source.end, (long long)source.first, (long long)source.start);
        };
        for(const auto &v:s.sensors) remember(v);
        for(const auto &v:s.readings) if(v.time>=v.start && v.time<=now && v.mgdl>=20 && v.mgdl<=600)
            remember({v.sensor,v.start,v.start+v.sessionLength,v.time});
        if (r.sources != state.get().sources) commit(r, &discovery);
        if(r.pending) {
            auto p=pending(); diagnostic("retrying the persisted pending batch");
            log("retry",p); logGlucose("pending glucose",range(p.times)); return true;
        }
        // Retention proofs change independently of uploads. Append only those
        // records whose retained source position actually changed.
        r = state.get();
        MappedIndexEdit positions(data, r.end);
        const MappedIndexView committed(data, state.get().end);
        bool positionsChanged = false;
        if (sendNumbers) committed.visit(r.eventIndex, [&](const IndexNode &entry) {
            auto saved = data.get<EventNode>(entry.value, r.end);
            if (saved.deleted) return true;
            const auto key = data.get(saved.key, r.end);
            const auto before = saved.firstPosition;
            for (const auto &inv:s.numberInventories)
                if (inv.keys.contains(key)) saved.firstPosition = inv.firstPosition;
            if (saved.firstPosition != before) {
                // Reuse the mapped strings/content. Only the changed native
                // retention field and index value need to be written.
                saved.previous = r.events;
                r.events = data.put(r.end, saved);
                r.eventIndex = positions.putString(r.eventIndex, saved.key, r.events);
                positionsChanged = true;
            }
            return true;
        });
        if (positionsChanged) commit(r, &positions);
        r = state.get();
        const MappedIndexView index(data, r.end);
        auto occupied = [&](int64_t time) {
            SentNeighbours result;
            if (auto before = index.floor(r.timeIndex, time, false)) result.before = before->number;
            if (auto after = index.ceiling(r.timeIndex, time)) result.atOrAfter = after->number;
            return result;
        };
        auto selected = [&](const Reading &reading) {
            auto active = index.floor(r.timelineIndex, reading.time);
            return active && data.get(data.get<SensorNode>(active->value, r.end).sensor, r.end) == reading.sensor;
        };
        auto readings = selectIndexedReadings(s.readings, since, now, 256, occupied, selected);
        Json groups=Json::object();
        auto add=[&](const char *type,Json record) {
            if(!groups.contains(type)) groups[type]=Json::array();
            groups[type].push_back(std::move(record));
        };
        PendingBatch p;
        std::set<std::string> newSessions;
        std::string tx;
        for(const auto &v:readings) {
            tx=transmitterId(installation,v.sensor);
            if(!index.findString(r.sessionIndex, v.sensor) && newSessions.insert(v.sensor).second) {
                auto session=sessionRecord(v,tx); add("SensorSessionRecord",session);
                session["SessionState"]="InSession"; session["TransmitterTime"]=79+v.time-v.start;
                session["RecordedSystemTime"]=timestamp(v.time); session["RecordedDisplayTime"]=timestamp(v.time,true);
                add("SensorSessionRecord",std::move(session)); p.sessions.push_back(v.sensor);
            }
            add("GlucoseRecord",glucoseRecord(v,tx)); p.times.push_back(v.time);
        }
        if(sendNumbers) {
            std::set<std::string, std::less<>> live;
            for(const auto &n:s.numbers) live.insert(n.key);
            index.visit(r.eventIndex, [&](const IndexNode &entry) {
                const auto &saved=data.get<EventNode>(entry.value,r.end);
                const auto key=data.get(saved.key,r.end);
                if(saved.deleted || live.contains(key)) return true;
                bool covered=false,missing=true;
                for(const auto &inv:s.numberInventories) if(key.starts_with(inv.source+"/")) {
                    covered=true; missing &= inv.missing(key,saved.firstPosition==inv.firstPosition);
                }
                if(!covered || !missing) return true;
                auto old=event(entry.value); // Materialize only an actual outgoing deletion.
                if(old.revision==UINT64_MAX) throw Error("Clarity event revision exhausted");
                old.deleted=true; ++old.revision;
                add("UserEventRecord",eventRecord(old.content,now,"Deleted")); p.events.push_back(std::move(old));
                return p.events.size() < 256;
            });
            for(const auto &n:s.numbers) {
                if(p.events.size()==256) break;
                if(n.kind==NumberKind::Ignore || n.time<since || n.time>now) continue;
                if(!std::isfinite(n.value) ||
                   ((n.kind==NumberKind::Rapid || n.kind==NumberKind::Long || n.kind==NumberKind::Carbs) && n.value<0) ||
                   (n.kind==NumberKind::Blood && n.value<=0) ||
                   (n.kind==NumberKind::Carbs && (!std::isfinite(n.weight) || n.weight<=0))) {
                    diagnostic("error: skipping invalid number amount or category weight (kind=%d)",int(n.kind)); continue;
                }
                EventNode old{};
                const auto found=index.findString(r.eventIndex, n.key);
                if(found) old=data.get<EventNode>(found,r.end);
                const bool active=found && !old.deleted;
                auto name=installation+"/event/"+n.key;
                if(old.revision) name+="/revision/"+std::to_string(old.revision);
                auto content=eventContent(n,active?uuidString(old.id):stableUuid(name));
                if(active && content.time==old.time && uint32_t(content.kind)==old.kind &&
                   content.displayTime==data.get(old.displayTime,r.end) &&
                   content.value==data.get(old.value,r.end) &&
                   content.description==data.get(old.description,r.end)) continue;
                SavedEvent change{n.key,std::move(content),old.revision,-1,false,active};
                for(const auto &inv:s.numberInventories) if(inv.keys.contains(n.key)) change.firstPosition=inv.firstPosition;
                add("UserEventRecord",eventRecord(change.content,now,active?"Updated":"New")); p.events.push_back(std::move(change));
            }
        }
        if(groups.empty()) return false;
        r.sequence=static_cast<int32_t>(uint32_t(r.sequence)+1);
        // JSON here IS the protocol payload. Store its exact bytes in a reusable
        // mapped request buffer; retries feed those bytes directly to encryption.
        auto body=makePost(account,installation,r.sequence,groups,tx).dump();
        if(body.size()>16*1024*1024) throw Error("Clarity pending request exceeds size limit");
        wire.ensure(body.size()); memcpy(wire.data(),body.data(),body.size());
        try { wire.sync(0,body.size()); } catch(...) {failed=true;throw;}
        PendingNode node{}; p.id=uuid(); uuidField(node.id,p.id);
        node.times=r.times; node.events=r.events; node.sessions=r.sessions;
        if(!p.times.empty()) {
            TimeNode t{}; t.previous=r.times;
            t.values={data.append(r.end,p.times.data(),p.times.size()*sizeof(int64_t)),uint32_t(p.times.size()*sizeof(int64_t)),0};
            node.times=data.put(r.end,t);
        }
        for(const auto &name:p.sessions) {
            SessionNode t{}; t.previous=node.sessions; t.sensor=data.put(r.end,std::string_view(name));
            node.sessions=data.put(r.end,t);
        }
        for(const auto &e:p.events) node.events=putEvent(r,e,node.events);
        auto bounds=range(p.times); node.count=bounds.count; node.first=bounds.first; node.last=bounds.last;
        node.wireSize=body.size(); node.wireChecksum=mappedChecksum(body.data(),body.size());
        r.pending=data.put(r.end,node); commit(r);
        diagnostic("batch saved: glucose=%zu sessions=%zu events=%zu mapped bytes=%llu",p.times.size(),p.sessions.size(),p.events.size(),
                   (unsigned long long)r.end);
        log("pending",p); logGlucose("pending glucose",bounds); return true;
    }
    void acknowledge(const std::string &receipt, bool alreadyStored = false) {
        healthy(); auto r=state.get();
        if((!alreadyStored && !isUuid(receipt)) || !r.pending) throw Error("Invalid Clarity acknowledgment");
        if (alreadyStored) {
            // A duplicate confirms our retained post, not a newly selected
            // batch. Check its protocol identity before advancing native state.
            const auto saved = Json::parse(pendingBody());
            const auto &post = saved.at("DataPost");
            const auto envelope = Json::parse(post.at("PostHeader").get<std::string>());
            const auto &content = envelope.at("Content").get_ref<const std::string &>();
            auto mac = hmac256(bytes("DxDs" + account), bytes(content));
            mac.resize(16);
            const auto header = Json::parse(content);
            if (post.at("AccountID") != account || post.at("SequenceID") != r.sequence ||
                envelope.at("IsZip") != false || envelope.at("Hmac") != base64(mac) ||
                header.at("PatientId") != account || header.at("InstallationId") != installation ||
                header.at("SequenceNumber") != r.sequence)
                throw Error("Clarity duplicate confirmation does not match the retained post identity");
        }
        const auto p=pending();
        const auto node=data.get<PendingNode>(r.pending,r.end); // Copy before any remap.
        MappedIndexView before(data, r.end);
        std::vector<SavedEvent> previousValues;
        for (const auto &e:p.events) if(e.updated && !e.deleted)
            if (auto pos=before.findString(r.eventIndex,e.key)) previousValues.push_back(event(pos));
        MappedIndexEdit edit(data, r.end);
        for (auto t:p.times) {
            const auto prior=edit.floor(r.timeIndex,t);
            if(prior && prior->number==t) throw Error("Duplicate mapped Clarity acknowledgment time");
            r.timeIndex=edit.putNumber(r.timeIndex,t,1);
        }
        walk<SessionNode>(node.sessions,r.sessions,[&](const SessionNode &n,uint64_t pos) {
            r.sessionIndex=edit.putString(r.sessionIndex,n.sensor,pos);
        });
        std::set<std::string> changedKeys; // This batch only, at most 256 keys.
        walk<EventNode>(node.events,r.events,[&](const EventNode &n,uint64_t pos) {
            if(changedKeys.insert(string(n.key)).second) r.eventIndex=edit.putString(r.eventIndex,n.key,pos);
        });
        r.times=node.times; r.events=node.events; r.sessions=node.sessions;
        r.count+=node.count;
        if(node.count) { if(!r.first || node.first<r.first) r.first=node.first; r.last=std::max(r.last,node.last); }
        if (!alreadyStored) uuidField(r.receipt,receipt);
        r.pending=0;
        commit(r,&edit);
        if (alreadyStored)
            diagnostic("duplicate post confirmation saved: batch=%s sequence=%d; pending batch cleared; previous signed receipt retained",
                       p.id.c_str(), r.sequence);
        else
            diagnostic("batch acknowledgment saved: batch=%s receipt=%s; pending batch cleared",p.id.c_str(),receipt.c_str());
        log("acknowledged",p,&previousValues); logGlucose("acknowledged batch glucose",range(p.times));
        diagnostic("acknowledged glucose total: count=%lld",(long long)state.get().count);
    }
};
Outbox::Outbox(std::string path,const std::string &a,const std::string &i):impl(std::make_unique<Impl>(path,a,i)) {}
Outbox::~Outbox()=default;
std::string Outbox::savedInstallation(const std::string &path,const std::string &account) {
    MappedCheckpoint<Root> saved(path,"G7OUTB02",initial(account,uuid()),false);
    if(uuidString(saved.get().account)!=account) throw Error("Clarity outbox identity mismatch");
    return uuidString(saved.get().installation);
}
bool Outbox::prepare(const Snapshot &s,int64_t since,int64_t now,bool numbers) {return impl->prepare(s,since,now,numbers);}
PendingBatch Outbox::pending() const {return impl->pending();}
std::string_view Outbox::pendingBody() const {return impl->pendingBody();}
void Outbox::acknowledge(const std::string &receipt) {impl->acknowledge(receipt);}
void Outbox::acknowledgeAlreadyStored() {impl->acknowledge({},true);}
size_t Outbox::sentGlucose() const {return impl->state.get().count;}
const std::string &Outbox::account() const {return impl->account;}
const std::string &Outbox::installation() const {return impl->installation;}
GlucoseProgress Outbox::glucoseProgress() const {
    const auto &r=impl->state.get();
    if(!r.pending) return {{r.count,r.first,r.last},{}};
    const auto &p=impl->data.get<PendingNode>(r.pending,r.end);
    return {{r.count,r.first,r.last},{p.count,p.first,p.last}};
}
std::vector<SensorSource> Outbox::sensorSources() const {
    // Explicit diagnostic/test snapshot only; never called by opening or selection.
    std::vector<SensorSource> result;
    const auto &r=impl->state.get();
    MappedIndexView index(impl->data,r.end);
    index.visit(r.sourceIndex,[&](const IndexNode &entry) {
        const auto &n=impl->data.get<SensorNode>(entry.value,r.end);
        result.push_back({impl->string(n.sensor),n.start,n.end,n.first});
        return true;
    });
    return result;
}
std::vector<SavedEvent> Outbox::savedEvents() const {
    // Explicit diagnostic/test snapshot only; never called by opening or lookup.
    std::vector<SavedEvent> result;
    const auto &r=impl->state.get();
    MappedIndexView index(impl->data,r.end);
    index.visit(r.eventIndex,[&](const IndexNode &entry) {
        result.push_back(impl->event(entry.value)); return true;
    });
    return result;
}
} // namespace clarity
