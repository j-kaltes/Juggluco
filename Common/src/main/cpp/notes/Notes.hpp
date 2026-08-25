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

#pragma once

#ifdef USE_FREETEXT_NOTE

#include "inout.hpp"
#include <string.h>
#include <algorithm>

extern std::string_view globalbasedir;

inline static constexpr const char notesdat[]="notes.dat";
constexpr const int notesstartsize=16384;
constexpr const int notemaxlen=128;

struct NoteEntry {
    uint32_t time;
    uint16_t textlen;
    char text[];   // flexible array: textlen bytes + null terminator

    static size_t totalsize(uint16_t textlen) {
        return sizeof(NoteEntry) + textlen + 1;
    }
    const char* gettext() const {
        return text;
    }
    size_t entrysize() const {
        return totalsize(textlen);
    }
};

struct NoteHeader {
    uint32_t count;
    uint32_t datastart;    // byte offset from start of file to first NoteEntry
    uint32_t nextfree;     // byte offset from datastart to next free byte
    uint32_t capacity;     // total capacity of data area in bytes
};

class Notes: public Mmap<uint8_t> {
    NoteHeader* header() {
        return reinterpret_cast<NoteHeader*>(Mmap::data());
    }
    const NoteHeader* header() const {
        return reinterpret_cast<const NoteHeader*>(Mmap::data());
    }
    uint8_t* dataarea() {
        return Mmap::data() + header()->datastart;
    }
    const uint8_t* dataarea() const {
        return Mmap::data() + header()->datastart;
    }

public:
    Notes(): Mmap(globalbasedir, notesdat, notesstartsize) {
        if(auto *h=header()) {
            if(h->datastart==0) {
                h->count=0;
                h->datastart=sizeof(NoteHeader);
                h->nextfree=0;
                h->capacity=notesstartsize-sizeof(NoteHeader);
            }
        }
    }

    NoteEntry* first() {
        return reinterpret_cast<NoteEntry*>(dataarea());
    }
    const NoteEntry* first() const {
        return reinterpret_cast<const NoteEntry*>(dataarea());
    }

    NoteEntry* entryat(uint32_t offset) {
        return reinterpret_cast<NoteEntry*>(dataarea() + offset);
    }
    const NoteEntry* entryat(uint32_t offset) const {
        return reinterpret_cast<const NoteEntry*>(dataarea() + offset);
    }

    uint32_t count() const {
        return header()->count;
    }

    const char* gettext(uint32_t offset) const {
        if(offset >= header()->nextfree)
            return "";
        return entryat(offset)->gettext();
    }

    uint32_t addnote(uint32_t time, const char* text, uint16_t textlen) {
        if(textlen > notemaxlen)
            textlen = notemaxlen;
        size_t entrysz = NoteEntry::totalsize(textlen);
        auto *h = header();

        if(h->nextfree + entrysz > h->capacity) {
            if(!compact())
                return UINT32_MAX;
        }

        uint32_t offset = h->nextfree;
        NoteEntry* e = entryat(offset);
        e->time = time;
        e->textlen = textlen;
        memcpy(e->text, text, textlen);
        e->text[textlen] = '\0';

        h->nextfree += entrysz;
        h->count++;
        return offset;
    }

    void updatenote(uint32_t offset, uint32_t time, const char* text, uint16_t textlen) {
        if(offset >= header()->nextfree)
            return;
        NoteEntry* e = entryat(offset);
        if(textlen > notemaxlen)
            textlen = notemaxlen;
        if(e->textlen == textlen) {
            e->time = time;
            memcpy(e->text, text, textlen);
            e->text[textlen] = '\0';
        } else {
            removeat(offset);
            addnote(time, text, textlen);
        }
    }

    void removeat(uint32_t offset) {
        if(offset >= header()->nextfree)
            return;
        NoteEntry* e = entryat(offset);
        size_t esz = e->entrysize();
        auto *h = header();
        memmove(dataarea() + offset, dataarea() + offset + esz,
                h->nextfree - offset - esz);
        h->nextfree -= esz;
        h->count--;
    }

    void purgenotes(uint32_t cutoff_time) {
        auto *h = header();
        uint32_t offset = 0;
        while(offset < h->nextfree) {
            NoteEntry* e = entryat(offset);
            if(e->time < cutoff_time) {
                size_t esz = e->entrysize();
                memmove(dataarea() + offset, dataarea() + offset + esz,
                        h->nextfree - offset - esz);
                h->nextfree -= esz;
                h->count--;
            } else {
                offset += e->entrysize();
            }
        }
    }

    bool compact() {
        auto *h = header();
        uint32_t oldoffset = 0;
        uint32_t newoffset = 0;
        uint8_t* src = dataarea();
        uint8_t* dst = dataarea();
        uint32_t count = 0;

        while(oldoffset < h->nextfree) {
            NoteEntry* e = reinterpret_cast<NoteEntry*>(src + oldoffset);
            size_t esz = e->entrysize();
            if(newoffset != oldoffset) {
                memmove(dst + newoffset, src + oldoffset, esz);
            }
            newoffset += esz;
            oldoffset += esz;
            count++;
        }
        h->nextfree = newoffset;
        h->count = count;
        return true;
    }

    int32_t findbytime(uint32_t time) const {
        auto *h = header();
        uint32_t offset = 0;
        while(offset < h->nextfree) {
            const NoteEntry* e = entryat(offset);
            if(e->time == time)
                return offset;
            offset += e->entrysize();
        }
        return -1;
    }
};

#endif
