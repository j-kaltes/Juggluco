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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

package tk.glucodata

import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import java.io.Closeable

/** Metadata only, kept locally with Health Connect cursors rather than mirrored. */
internal class ActivityChangeIndex(context:Context,generation:Long):ActivityRecordIntervals,Closeable {
    private val helper=object:SQLiteOpenHelper(context.applicationContext,"activity-changes.db",null,1) {
        override fun onCreate(db:SQLiteDatabase) {
            db.execSQL("CREATE TABLE records(kind INTEGER NOT NULL,id TEXT NOT NULL,start INTEGER NOT NULL,end INTEGER NOT NULL,origin TEXT NOT NULL,seen INTEGER NOT NULL,PRIMARY KEY(kind,id))")
            db.execSQL("CREATE TABLE indexed(kind INTEGER PRIMARY KEY)")
            db.execSQL("CREATE TABLE state(name TEXT PRIMARY KEY,value INTEGER NOT NULL)")
        }
        override fun onUpgrade(db:SQLiteDatabase,oldVersion:Int,newVersion:Int) {}
    }
    private val db=helper.writableDatabase
    init {
        val previous=db.rawQuery("SELECT value FROM state WHERE name='generation'",null).use {
            if(it.moveToFirst())it.getLong(0) else null
        }
        if(previous!=generation)transaction {
            db.delete("records",null,null);db.delete("indexed",null,null);db.delete("state",null,null)
            db.execSQL("INSERT INTO state(name,value) VALUES('generation',?)",arrayOf(generation))
            db.execSQL("INSERT INTO state(name,value) VALUES('scan',0)")
        }
    }
    private inline fun transaction(action:()->Unit) {
        // All work inside this function is synchronous. An Android SQLite
        // transaction must never be held across a suspending Health Connect call.
        db.beginTransaction()
        try {action();db.setTransactionSuccessful()} finally {db.endTransaction()}
    }
    fun indexed(kind:Int)=db.rawQuery("SELECT 1 FROM indexed WHERE kind=?",arrayOf(kind.toString())).use {it.moveToFirst()}
    fun beginSnapshot(kind:Int):Long {
        var scan=0L
        transaction {
            db.delete("indexed","kind=?",arrayOf(kind.toString()))
            db.execSQL("UPDATE state SET value=value+1 WHERE name='scan'")
            scan=db.rawQuery("SELECT value FROM state WHERE name='scan'",null).use {it.moveToFirst();it.getLong(0)}
        }
        return scan
    }
    fun snapshotPage(kind:Int,scan:Long,records:List<Pair<String,ActivityRecordRange>>) {
        transaction {
            db.compileStatement("INSERT OR REPLACE INTO records(kind,id,start,end,origin,seen) VALUES(?,?,?,?,?,?)").use {statement->
                for((id,range) in records) {
                    statement.clearBindings();statement.bindLong(1,kind.toLong());statement.bindString(2,id)
                    statement.bindLong(3,range.start);statement.bindLong(4,range.end)
                    statement.bindString(5,range.origin);statement.bindLong(6,scan);statement.executeInsert()
                }
            }
        }
    }
    fun finishSnapshot(kind:Int,scan:Long) {
        transaction {
            db.delete("records","kind=? AND seen<>?",arrayOf(kind.toString(),scan.toString()))
            db.execSQL("INSERT OR REPLACE INTO indexed(kind) VALUES(?)",arrayOf(kind))
        }
    }
    override fun find(kind:Int,id:String)=db.rawQuery("SELECT start,end,origin FROM records WHERE kind=? AND id=?",arrayOf(kind.toString(),id)).use {
        if(it.moveToFirst())ActivityRecordRange(it.getLong(0),it.getLong(1),it.getString(2)) else null
    }
    override fun put(kind:Int,id:String,range:ActivityRecordRange) {
        db.execSQL("INSERT OR REPLACE INTO records(kind,id,start,end,origin,seen) VALUES(?,?,?,?,?,0)",
            arrayOf<Any>(kind,id,range.start,range.end,range.origin))
    }
    override fun remove(kind:Int,id:String) {db.delete("records","kind=? AND id=?",arrayOf(kind.toString(),id))}
    override fun close() {helper.close()}
}
