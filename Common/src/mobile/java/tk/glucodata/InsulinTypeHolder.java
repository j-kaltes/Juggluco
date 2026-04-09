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
/*                                                                                   */
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata;

import android.app.Activity;
import android.content.Context;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;
import androidx.recyclerview.widget.RecyclerView;
import tk.glucodata.Applic;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.GlucoseCurve.getheight;
import static tk.glucodata.GlucoseCurve.getwidth;
import static tk.glucodata.Layout.getMargins;
import static tk.glucodata.MainActivity.systembarBottom;
import static tk.glucodata.MainActivity.systembarLeft;
import static tk.glucodata.MainActivity.systembarRight;
import static tk.glucodata.MainActivity.systembarTop;
import static tk.glucodata.Natives.canSendNumbers;
import static tk.glucodata.Natives.getInsulinType;
import static tk.glucodata.Natives.setInsulinType;
import static tk.glucodata.Natives.setlibrenum;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.edit2float;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.float2string;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.getradiobuttonId;
import java.util.ArrayList;


public class InsulinTypeHolder extends RecyclerView.ViewHolder {


private static void settype(MainActivity act, IOB.InsulinTypeAdapter adapter, int index,String name) {
    var group=new RadioGroup(act);
    int id=0;
    group.addView(getradiobuttonId(act,R.string.not,id++));
    group.addView(getradiobuttonId(act,R.string.humaninsulin,id++));
    group.addView(getradiobuttonId(act,R.string.aspart,id++));
    group.addView(getradiobuttonId(act,R.string.lispro,id++));
    group.addView(getradiobuttonId(act,R.string.glulisine,id++));
    group.addView(getradiobuttonId(act,R.string.fiasp,id++));
    group.addView(getradiobuttonId(act,R.string.urli,id++));
    group.addView(getradiobuttonId(act,R.string.afrezza,id));

    group.check(getInsulinType(index));

    group.setOnCheckedChangeListener( (g,i)-> {
        setInsulinType(index,i);
         });
//    onCheckedChanged (RadioGroup group, int checkedId)
  var nameview=getlabel(act,name);
   nameview.setTextSize(TypedValue.COMPLEX_UNIT_PX,Applic.headfontsize*.3f);
   var ok=getbutton(act, R.string.ok);
   var butlay=new Layout(act,(x,w,h)->{
      /*  x.setX(0);
        x.setY(0); */
         return new int[] {w,h};
           },new View[]{nameview},new View[]{ok});
    var mar=getMargins(ok);
   var density= tk.glucodata.GlucoseCurve.metrics.density;
    mar.topMargin=(int)(getheight()*.3f);
    mar.bottomMargin=(int)(density*30.0f);
    butlay.usebaseline=false;
   var lay=new Layout(act,(x,w,h)->{
         return new int[] {w,h};
           },new View[]{butlay,group});
    lay.usebaseline=false;
    lay.setBackgroundColor(Applic.backgroundcolor);
   MainActivity.setonback(() -> {
      removeContentView(lay);
      adapter.notifyItemChanged(index);
      });
   ok.setOnClickListener(v-> {
        MainActivity.doonback();
        });
   lay.setPadding(MainActivity.systembarLeft+(int)(15.0*density),MainActivity.systembarTop+(int)(4.0*density),MainActivity.systembarRight+(int)(18.0*density),(int)(4.0*density)+MainActivity.systembarBottom);

    act.addContentView(lay, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
    }


    public InsulinTypeHolder(View labelview, MainActivity act, IOB.InsulinTypeAdapter adapter, ArrayList<String > labels) {
    super(labelview);
    labelview.setOnClickListener(v -> {
        int pos=getAbsoluteAdapterPosition();
        settype(act, adapter,pos,labels.get(pos));
        });

    }

}


