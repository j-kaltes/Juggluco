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

// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

import androidx.appcompat.app.AlertDialog;

import android.app.DatePickerDialog;
import android.view.View;
import android.view.ViewGroup;
import java.util.Calendar;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import static tk.glucodata.help.help;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.settings.Settings.removeContentView;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;

public final class Clarity {
    private static final String LOG_ID="Clarity";
    private static String statusText(MainActivity act,ClarityStatus state) {
        String message=state.status;
        var format=new SimpleDateFormat("yyyy-MM-dd HH:mm",Locale.ROOT);
        if(state.lastSuccess>0)
            message+="\n"+act.getString(R.string.clarity_last_ack,format.format(new Date(state.lastSuccess*1000L)));
        message+="\n"+act.getString(R.string.clarity_ack_count,state.acknowledgedCount);
        if(state.acknowledgedCount>0)
            message+="\n"+act.getString(R.string.clarity_ack_latest,format.format(new Date(state.acknowledgedLast*1000L)));
        if(state.pendingCount>0)
            message+="\n"+act.getString(R.string.clarity_pending_glucose,state.pendingCount,format.format(new Date(state.pendingFirst*1000L)),format.format(new Date(state.pendingLast*1000L)));
        return message;
    }
    static void logFailure(String operation, Throwable error) {
        logFailure(LOG_ID,operation,error);
    }
    static void logFailure(String logId,String operation,Throwable error) {
        // Exception messages (especially JSON/URI errors) may contain tokens.
        Log.e(logId, operation + ": " + error.getClass().getName());
        StackTraceElement[] frames=error.getStackTrace();
        for(int i=0;i<Math.min(3,frames.length);++i)
            Log.e(logId, "at " + frames[i]);
    }
    public static void show(MainActivity act,View parent) {
        final ClarityStatus state;
        try{
            state=Natives.clarityStatus();
            if(state==null)throw new IllegalStateException();
            }
        catch(Exception ex){
            logFailure("Read settings",ex);
            Applic.Toaster(act.getString(R.string.clarity_read_settings_error));
            return;
            }
        if(!state.available) {
            Log.e(LOG_ID,"Native Clarity support unavailable");
            Applic.Toaster(state.status);
            return;
            }
        var enabled=getcheckbox(act,act.getString(R.string.clarity_send_enabled),state.enabled);
        var history=getcheckbox(act,act.getString(R.string.clarity_use_history),state.libre3History);
        var numbers=getcheckbox(act,R.string.sendamounts,state.numbers);
        var account=getlabel(act,state.account);
        var status=getlabel(act,statusText(act,state));
        final long[] since={state.since};
        var format=new SimpleDateFormat("yyyy-MM-dd",Locale.ROOT);
        var date=getlabel(act,act.getString(R.string.clarity_start_date,format.format(new Date(since[0]*1000L))));
        var changestart=getbutton(act,R.string.changestartbutton);
        changestart.setOnClickListener(v->{
            var selected=Calendar.getInstance();
            selected.setTimeInMillis(since[0]*1000L);
            var picker=new DatePickerDialog(act,(view,year,month,day)->{
                var chosen=Calendar.getInstance();
                chosen.clear();
                chosen.set(year,month,day);
                final long candidate=chosen.getTimeInMillis()/1000L;
                if(candidate==since[0])return;
                new AlertDialog.Builder(act)
                    .setTitle(act.getString(R.string.clarity_change_start_title))
                    .setMessage(act.getString(R.string.clarity_change_start_message,format.format(chosen.getTime())))
                    .setNegativeButton(R.string.cancel,null)
                    .setPositiveButton(android.R.string.ok,(dialog,which)->{
                        since[0]=candidate;
                        date.setText(act.getString(R.string.clarity_start_date,format.format(chosen.getTime())));
                    }).show();
            },selected.get(Calendar.YEAR),selected.get(Calendar.MONTH),selected.get(Calendar.DAY_OF_MONTH));
            picker.getDatePicker().setMinDate(1598911200000L);
            picker.getDatePicker().setMaxDate(System.currentTimeMillis());
            picker.show();
        });
        var categories=getbutton(act,act.getString(R.string.clarity_categories));
        var login=getbutton(act,act.getString(R.string.clarity_sign_in));
        var send=getbutton(act,R.string.sendnow);
        var save=getbutton(act,R.string.save);
        var close=getbutton(act,R.string.cancel);
        final var helpbutton=getbutton(act,R.string.helpname);
        helpbutton.setOnClickListener(v-> help(R.string.clarityhelp,act));
        status.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
        var layout=new Layout(act,(l,w,h)->new int[]{w,h},new View[]{date,changestart},new View[]{status},new View[]{history,numbers,categories},new View[]{enabled,login,account},new View[]{send,helpbutton,close,save}).portraitLayout(new View[]{date,changestart},new View[]{status},new View[]{enabled},new View[]{history},new View[]{numbers,categories},new View[]{login},new View[]{account},new View[]{send,helpbutton},new View[]{close,save});
        layout.setBackgroundColor(Applic.backgroundcolor);
        final Runnable refresh=()->{try{var s=Natives.clarityStatus();if(s==null)throw new IllegalStateException();account.setText(s.account);status.setText(statusText(act,s));}catch(Exception ex){logFailure("Refresh settings",ex);}};
        numbers.setOnCheckedChangeListener((button,checked)->{
            if(checked && !ClarityAmounts.ready()) {
                numbers.setChecked(false);
                ClarityAmounts.show(act,layout,()->numbers.setChecked(true));
            }
        });
        categories.setOnClickListener(v->ClarityAmounts.show(act,layout,()->{}));
        Runnable done=()->{
            removeContentView(layout);
            parent.setVisibility(View.VISIBLE);};
        close.setOnClickListener(v->act.doonback());
        login.setOnClickListener(v->ClarityLogin.show(act,layout,refresh));
        send.setOnClickListener(v->{Natives.clarityWake();refresh.run();});
        save.setOnClickListener(v->{
        try {
            if(numbers.isChecked() && !ClarityAmounts.ready()) {
                numbers.setChecked(false);
                ClarityAmounts.show(act,layout,()->numbers.setChecked(true));
                return;
            }
            String error=Natives.clarityConfigure(enabled.isChecked(),history.isChecked(),numbers.isChecked(),since[0]);
            if(error.isEmpty())
                act.doonback();
            else 
                Applic.Toaster(error);
             }    
         catch(Exception ex){
            logFailure("Save settings",ex);
            Applic.Toaster(act.getString(R.string.clarity_save_settings_error));
            }});
        parent.setVisibility(View.GONE);
        final int sidepad=(int)(GlucoseCurve.metrics.density*8);
        layout.systembarPadding((left,top,right,bottom)-> {
              return   new int[]{left+sidepad,top,sidepad+right,bottom};
              });

        act.addMyContentView(layout,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
        MainActivity.setonback(done);
    }
}
