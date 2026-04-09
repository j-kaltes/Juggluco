/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+ and           */
/*      Sibionics GS1Sb sensors.                                                     */
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
/*      Sat Jul 26 11:06:06 CEST 2025                                                */
package tk.glucodata;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static tk.glucodata.Natives.getInvertColors;
import static tk.glucodata.Natives.getshowcalibratedscans;
import static tk.glucodata.Natives.getshowcalibratedstream;
import static tk.glucodata.Natives.getshowhistories;
import static tk.glucodata.Natives.getshownumbers;
import static tk.glucodata.Natives.getshowscans;
import static tk.glucodata.Natives.getshowstream;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;

import android.view.View;
import android.view.ViewGroup;

import android.widget.ScrollView;

class Display {
static void show(MainActivity context) {
    var close=getbutton(context,R.string.closename);
     var Scans=getcheckbox(context,R.string.scansname,getshowscans()) ;
     var CalibratedStream=getcheckbox(context,context.getString(R.string.streamname)+" "+context.getString(R.string.calibrated),getshowcalibratedstream()) ;
     var CalibratedHistory=getcheckbox(context,context.getString(R.string.historyname)+" "+context.getString(R.string.calibrated), Natives.getshowcalibratedhistories());
     var CalibratedScans=getcheckbox(context,context.getString(R.string.scansname)+" "+context.getString(R.string.calibrated),getshowcalibratedscans()) ;
     var History=getcheckbox(context,R.string.historyname,getshowhistories()) ;
     var Stream=getcheckbox(context,R.string.streamname,getshowstream()) ;
     var Amounts=getcheckbox(context,R.string.amountshort,getshownumbers()) ;
    CheckDirectionBox darkmodeview=getcheckbox(context,R.string.darkmode,getInvertColors());
    darkmodeview.setOnCheckedChangeListener( (buttonView, isChecked)->{
        Natives.setInvertColors(isChecked);
        });
CalibratedScans.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowcalibratedscans(isChecked); });

CalibratedStream.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowcalibratedstream(isChecked); });
CalibratedHistory.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowcalibratedhistories(isChecked); });
Scans.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowscans(isChecked); });
    History.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowhistories(isChecked); });
    Stream.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshowstream(isChecked); });
    Amounts.setOnCheckedChangeListener( (buttonView,  isChecked) -> { Natives.setshownumbers(isChecked); });
     var layout=new Layout(context, (l, w, h) -> {
            final int[] ret={w,h};
            return ret;
     }, new View[]{Scans},new View[]{History},new View[]{Stream},new View[]{CalibratedScans},new View[]{CalibratedHistory},new View[]{CalibratedStream},new View[]{Amounts},new View[]{darkmodeview},new View[]{close});
    layout.setBackgroundColor(Applic.backgroundcolor);

    var height=GlucoseCurve.getheight();
    layout.setPadding(0,(int)(height*.1f),0,(int)(height*.01f));
    var scroll=new ScrollView(context);
    scroll.addView(layout);
    scroll.setFillViewport(true);
    scroll.setSmoothScrollingEnabled(false);
   scroll.setScrollbarFadingEnabled(true);
   scroll.setVerticalScrollBarEnabled(Applic.scrollbar);
    context.addContentView(scroll, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
    close.setOnClickListener(v -> {
        MainActivity.doonback();
        });
    MainActivity.setonback(() -> {
            removeContentView(scroll);
            context.requestRender();
            });
    }
};
