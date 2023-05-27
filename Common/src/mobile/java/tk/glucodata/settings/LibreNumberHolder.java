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


package tk.glucodata.settings;

import android.app.Activity;
import android.content.Context;
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
import tk.glucodata.measuredgrid;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.GlucoseCurve.getheight;
import static tk.glucodata.GlucoseCurve.getwidth;
import static tk.glucodata.Natives.canSendNumbers;
import static tk.glucodata.Natives.setlibrenum;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.edit2float;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.float2string;
import static tk.glucodata.settings.Settings.removeContentView;


public class LibreNumberHolder extends RecyclerView.ViewHolder {
	private ViewGroup parent;
	private 	View sendnumbers;
	private int night;
private static final String LOG_ID="LibreNumberHolder";
RadioButton mkradiobutton(Context context, String label) {
	  var radio  = new RadioButton(context);
	  radio.setText(label);
	  return radio;
	}
int getid(View layout,int res) {
	RadioButton longacting=layout.findViewById(res);
	final int longid=longacting.getId();
	return longid;
	}
static int[] kindids=null;
static int foodid;
int kindfromid(int id) {
	final var len=kindids.length;
	for(int i=0;i<len;i++) {
		if(id==kindids[i])
			return i;
		}
	return 0;
	}
void sendoptions(View labelview,int pos) {
	EnableControls(parent,false);
	var context=(MainActivity)(labelview.getContext());
 	LayoutInflater flater= LayoutInflater.from(context);
       var layout = flater.inflate(R.layout.librenumoptions, null, false);


         layout.setBackgroundResource(R.drawable.dialogbackground);
	 int laypad=(int)(GlucoseCurve.metrics.density*4.0f);
	 layout.setPadding(laypad,0,laypad,laypad);

        context.addContentView(layout, new ViewGroup.LayoutParams( WRAP_CONTENT, WRAP_CONTENT));
         Button close=layout.findViewById(R.id.close);
	Button save=layout.findViewById(R.id.save);
	TextView label=layout.findViewById(R.id.label);
		 RadioGroup group=layout.findViewById(R.id.group);
	View weightlabel=layout.findViewById(R.id.weightlabel);
	EditText weight=layout.findViewById(R.id.weight);
	weight.setImeOptions(editoptions);
         Button help=layout.findViewById(R.id.help);
	help.setOnClickListener(v->{tk.glucodata.help.basehelp(context.getString(R.string.setlibrenumtype),context,l-> { } , (hv,w,h)-> {return new int[] {
	getwidth(),getheight() };} , new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT)); }); ;

	measuredgrid grid=layout.findViewById(R.id.librenum);
	grid.setmeasure((l,w,h)-> {
		int height= getheight();
		int width= getwidth();
		l.setY(0);
		int x=(width>w)?((width-w)/2):0;
		l.setX(x);
	});

	if(kindids==null) {
		 final int rapidactingid=getid(layout,R.id.rapidacting);
		 final int longactingid=getid(layout,R.id.longacting);
		 foodid=getid(layout,R.id.food);
		 final int noteid=getid(layout,R.id.note);
		 final int dontid=getid(layout,R.id.dont);
		 kindids=new int[]{-1,rapidactingid,longactingid,foodid,noteid,dontid};
		 }
	final int wasid=kindids[Natives.getlibrenumkind(night,pos)];
	group.check(wasid);
	if(wasid==foodid) {
		float wasweight=Natives.getlibrefoodweight(night,pos);
		weight.setText(float2string(wasweight));
		}
	else {
		weight.setVisibility(INVISIBLE);
		weightlabel.setVisibility(INVISIBLE);
		save.setVisibility(INVISIBLE);
		}
	final int[] newid={wasid};
	group.setOnCheckedChangeListener( (gr,id)-> {
				tk.glucodata.Log.i(LOG_ID, "id="+id);
			save.setVisibility(VISIBLE);
				if(foodid==id) {
					weight.setVisibility(VISIBLE);
					weightlabel.setVisibility(VISIBLE);
					}
				else {
					weight.setVisibility(INVISIBLE);
					weightlabel.setVisibility(INVISIBLE);

				}
				newid[0]=id;
				 });
	 label.setText(((TextView)labelview).getText());

	Runnable closerun=()-> {
		EnableControls(parent,true);
		hidekeyboard((Activity) context);
		removeContentView(layout) ;
		if(!canSendNumbers(night)) {
			EnableControls(sendnumbers,false);
			}
		};
	context.setonback(closerun);
         close.setOnClickListener(v-> context.doonback());
	save.setOnClickListener(v-> {
		final int theid=newid[0];
		if(theid!=wasid||wasid==foodid) {
			final float weightf=(theid==foodid)?edit2float(weight):0.0f;
			final int kind=kindfromid(theid);
			setlibrenum(night,pos,kind, weightf);
			}
		context.doonback();
	});


}


public LibreNumberHolder(View labelview,ViewGroup parent,View sendnumbers,int night) {
        super(labelview);
	this.parent=parent;
	this.sendnumbers=sendnumbers;
	this.night=night;
       labelview.setOnClickListener(v -> {
		int pos=getAbsoluteAdapterPosition();
		sendoptions(v,pos);
		

		});

    }

}


/*
   var Rapid  = mkradiobutton(context,"Rapid acting insulin");
   var Long  = mkradiobutton(context,"Long acting insulin");
   var Food  = mkradiobutton(context,"Food");
   var Note  = mkradiobutton(context,"Note");
   var dont  = mkradiobutton(context,"Don't send");
   final ViewGroup  layout=new Layout(context,(x,w,h)->{ hideSystemUI(); return new int[] {w,h}; },new View[] {Rapid},new View[] {Long},new View[] {Food},new View[] {Note},new View[] {dont});*/
