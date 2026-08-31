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

#include "inout.hpp"
#include <string.h>
#include <algorithm>

extern std::string_view globalbasedir;

inline static constexpr const char notesdat[]="notes.dat";
constexpr const int notesstartsize=16384;
constexpr const int notemaxlen=128;
constexpr const int notemaxdisplay=10;
// The file is extended by this amount whenever it is full. notes.dat
// is a sparse file, so the unused tail costs nothing on disk.
constexpr const int notegrowsize=2*1024*1024;

// Shorten text to notemaxdisplay characters, appending "..." when truncated.
// buf must hold at least notemaxdisplay+4 bytes.
inline static void shortnotetext(const char* text, char* buf) {
    const int tlen = (int)strlen(text);
    if(tlen > notemaxdisplay) {
        memcpy(buf, text, notemaxdisplay);
        buf[notemaxdisplay]='.'; buf[notemaxdisplay+1]='.'; buf[notemaxdisplay+2]='.'; buf[notemaxdisplay+3]='\0';
        }
    else {
        memcpy(buf, text, tlen+1);
        }
    }

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
    // Grows the file by notegrowsize. Growing unmaps and remaps the
    // memory, so all pointers into it are invalid afterwards and must
    // be fetched again; existing entries keep their offsets, so stored
    // Num.mealptr values remain valid. When growing fails the mapping
    // is reconstructed at its current size, so no data is lost and the
    // caller simply reports that the new note was not created.
    bool grow() {
        const auto oldsize=size();
        extend(globalbasedir, notesdat, (int)(oldsize+notegrowsize));
        if(Mmap::data()==nullptr) {
            // Growing failed: keep the file at its current size.
            extend(globalbasedir, notesdat, (int)oldsize);
            if(Mmap::data()==nullptr)
                return false;
            }
        header()->capacity=size()-header()->datastart;
        return true;
        }

public:
    Notes(): Mmap(globalbasedir, notesdat, notesstartsize) {
        if(auto *h=header()) {
            if(h->datastart==0) {
                h->count=0;
                h->datastart=sizeof(NoteHeader);
                h->nextfree=0;
                }
            // Always derived from the actual mapping size, so files
            // extended after creation (see addnote) keep a matching
            // capacity.
            h->capacity=size()-h->datastart;
            }
        }

    NoteEntry* entryat(uint32_t offset) {
        return reinterpret_cast<NoteEntry*>(dataarea() + offset);
        }
    const NoteEntry* entryat(uint32_t offset) const {
        return reinterpret_cast<const NoteEntry*>(dataarea() + offset);
        }

    // Entries are tightly packed, so valid offsets are exactly the
    // cumulative entry boundaries. Walking the chain rejects any
    // foreign offset (e.g. a meal offset left in Num.mealptr).
    bool validlive(uint32_t offset) const {
        const auto *h = header();
        if(h==nullptr)
            return false;
        if(offset >= h->nextfree)
            return false;
        uint32_t pos = 0;
        while(pos < h->nextfree) {
            const NoteEntry* e = entryat(pos);
            if(pos == offset)
                return e->time != 0;        // live, not tombstoned
            pos += e->entrysize();          // >= 9, so this terminates
            }
        return false;
        }

    const char* gettext(uint32_t offset) const {
        if(!validlive(offset))
            return "";
        return entryat(offset)->gettext();
        }

    uint32_t addnote(uint32_t time, const char* text, uint16_t textlen) {
        if(textlen > notemaxlen)
            textlen = notemaxlen;
        size_t entrysz = NoteEntry::totalsize(textlen);
        auto *h = header();
        if(h==nullptr)
            return UINT32_MAX;

        // Reuse a tombstoned slot when it is large enough: keeps all
        // other offsets stable (Num.mealptr values stay valid).
        // The slot's original textlen is its capacity and must not
        // shrink, otherwise entry iteration would be misaligned.
        {
        uint32_t offset = 0;
        while(offset < h->nextfree) {
            NoteEntry* e = entryat(offset);
            size_t esz = e->entrysize();
            if(e->time == 0 && e->textlen >= textlen) {
                e->time = time;
                memcpy(e->text, text, textlen);
                e->text[textlen] = '\0';
                h->count++;
                return offset;
                }
            offset += esz;
            }
        }

        if(h->nextfree + entrysz > h->capacity) {
            // Full: grow the file. When growing fails the mapping is
            // kept at its current size, so no data is lost and the new
            // note is simply not created.
            if(!grow())
                return UINT32_MAX;
            h = header();
            }
        if(h->nextfree + entrysz > h->capacity)
            return UINT32_MAX;

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

    // Returns the offset actually storing the text. Equal-size updates
    // happen in place; otherwise the old entry is tombstoned and the
    // new text appended (or a tombstone slot reused), so offsets of
    // other notes never change.
    uint32_t updatenote(uint32_t offset, uint32_t time, const char* text, uint16_t textlen) {
        if(!validlive(offset))
            return UINT32_MAX;
        NoteEntry* e = entryat(offset);
        if(textlen > notemaxlen)
            textlen = notemaxlen;
        if(e->textlen == textlen) {
            e->time = time;
            memcpy(e->text, text, textlen);
            e->text[textlen] = '\0';
            return offset;
            }
        removeat(offset);
        return addnote(time, text, textlen);
        }

    // Tombstone the entry: offsets of all other notes remain valid.
    void removeat(uint32_t offset) {
        if(!validlive(offset))
            return;
        entryat(offset)->time = 0;
        header()->count--;
        }
    };
