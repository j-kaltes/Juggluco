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

package tk.glucodata;

import android.app.DatePickerDialog;
import android.content.ComponentCallbacks;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.DatePicker;
import java.util.Calendar;

/** Reinflate the platform picker's orientation-specific layout without committing a date. */
final class ActivityDatePicker implements ComponentCallbacks {
    private final Context context;
    private final DatePickerDialog.OnDateSetListener selected;
    private final Handler handler=new Handler(Looper.getMainLooper());
    private final Runnable reopen=this::open;
    private DatePickerDialog dialog;
    private Bundle state;
    private int year,month,day,orientation;
    private boolean active;
    ActivityDatePicker(Context context,DatePickerDialog.OnDateSetListener selected) {
        this.context=context;this.selected=selected;
    }
    void show(long seconds) {
        close();
        Calendar date=Calendar.getInstance();date.setTimeInMillis(seconds*1000);
        year=date.get(Calendar.YEAR);month=date.get(Calendar.MONTH);day=date.get(Calendar.DAY_OF_MONTH);
        orientation=context.getResources().getConfiguration().orientation;
        active=true;context.registerComponentCallbacks(this);
        open();
    }
    private void open() {
        if(!active)return;
        dialog=new DatePickerDialog(context,selected,year,month,day);
        dialog.setOnDismissListener(ignored->close());
        dialog.show();
        if(state!=null){dialog.onRestoreInstanceState(state);state=null;}
    }
    @Override public void onConfigurationChanged(Configuration config) {
        if(!active||config.orientation==orientation)return;
        orientation=config.orientation;
        if(dialog==null)return;
        DatePicker picker=dialog.getDatePicker();
        year=picker.getYear();month=picker.getMonth();day=picker.getDayOfMonth();
        state=dialog.onSaveInstanceState();
        dialog.setOnDismissListener(null);dialog.dismiss();dialog=null;
        // Let the Activity finish its existing configuration/geometry updates.
        handler.post(reopen);
    }
    void close() {
        handler.removeCallbacks(reopen);
        if(active){active=false;context.unregisterComponentCallbacks(this);}
        DatePickerDialog old=dialog;dialog=null;state=null;
        if(old!=null){old.setOnDismissListener(null);old.dismiss();}
    }
    @Override public void onLowMemory() {}
}
