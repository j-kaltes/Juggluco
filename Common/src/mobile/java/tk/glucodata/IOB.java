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
/*      Wed Mar 12 21:03:25 CET 2025                                                 */


package tk.glucodata;


import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Layout.getMargins;
import static tk.glucodata.Natives.getInsulinType;
import static tk.glucodata.Natives.setInsulinType;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.help;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getradiobutton;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

import tk.glucodata.InsulinTypeHolder;

public class IOB {


static public class InsulinTypeAdapter extends RecyclerView.Adapter<InsulinTypeHolder> {
    ArrayList<String > labels=Natives.getLabels();

    MainActivity act;;

    InsulinTypeAdapter(MainActivity act) {
    
        this.act=act;

        }

    @NonNull
    @Override
    public InsulinTypeHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        Button view=new Button( act);

        view.setTransformationMethod(null);
   //     view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);
       if(!isWearable)
               view.setTextSize(TypedValue.COMPLEX_UNIT_PX,Applic.largefontsize);
        view.setLayoutParams(new ViewGroup.LayoutParams(  ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        return new InsulinTypeHolder(view,act,this,labels);
    }

    @Override
    public void onBindViewHolder(final InsulinTypeHolder holder, int pos) {
        TextView text=(TextView)holder.itemView;
        var type=getInsulinType(pos);
        text.setText(labels.get(pos)+(type!=0?(": "+type):""));
        }
        @Override
        public int getItemCount() {
                return labels.size()-1;

        }

}


public static void mkview(MainActivity act) {
    RecyclerView recycle = new RecyclerView(act);
    recycle.setHasFixedSize(true);
    GridLayoutManager lin=new GridLayoutManager(act,3);
    recycle.setLayoutManager(lin);
    recycle.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    var adapt = new InsulinTypeAdapter(act);
    recycle.setAdapter(adapt);
    var iob=getcheckbox(act,"IOB",Natives.getIOB());
    var help=getbutton(act, R.string.helpname);
   help.setOnClickListener(v-> help(R.string.IOBhelp,act));
    var ok=getbutton(act, R.string.ok);
   var lay=new Layout(act,(x,w,h)->{
         return new int[] {w,h};
           },new View[]{recycle},new View[]{help,iob,ok});
   lay.setBackgroundColor(Applic.backgroundcolor);
   final var density= tk.glucodata.GlucoseCurve.metrics.density;
   var hormarg=(int)(0.15f*GlucoseCurve.getwidth());

   getMargins(help).setMarginStart(hormarg);
   getMargins(ok).setMarginEnd(hormarg);   
   lay.setPadding(MainActivity.systembarLeft+(int)(density*10.0f),MainActivity.systembarTop,MainActivity.systembarRight+(int)(density*10.0f),MainActivity.systembarBottom);
    iob.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
            if(!Natives.setIOB(isChecked)) {
                iob.setChecked(false);
                EnableControls(lay,false);
                help(R.string.IOBhelp,act,l->EnableControls(lay,true) );
                }
            }
        ); 
    act.addContentView(lay, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
   ok.setOnClickListener(v-> {
        MainActivity.doonback();
        });
    MainActivity.setonback(()-> removeContentView(lay));
    }

}
