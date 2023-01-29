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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */


package tk.glucodata;

import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.os.Build;
import android.text.InputType;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import tk.glucodata.settings.Settings;

import static android.provider.Settings.ACTION_NOTIFICATION_POLICY_ACCESS_SETTINGS;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Natives.getalarmdisturb;
import static tk.glucodata.help.help;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

public class RingTones {
 int kind;
 String uri=null;
 int duration,susp;
// Ringtone ringtone;
 TextView name;
CheckBox flashview;
Button permission;
private static final String LOG_ID="RingTones";

static public RingTones one=null;
public RingTones(int ki)  {
		one=this;
		kind=ki;
		uri= Natives.readring(kind);
		duration=Natives.readalarmduration(kind);
		susp=Natives.readalarmsuspension(kind);
		 //ringtone = Notify.getring(kind);
		 }
static 	public void setring(int ki,String uristr) {
	if(one!=null)  {
		one.setringer(ki,uristr);
		}

}
private static String gettitle(Context context,String uri,int kind) {
	try {
		Ringtone ringtone = Notify.mkrings(uri, kind);
		return ringtone.getTitle(context);
	} catch (Throwable th) {
		Log.stack(LOG_ID, "ringtone title", th);
		return "Unknown";
	}
}
void setringer(int ki,String uristr) {
	if(ki==kind) {
		uri=uristr;
		if(name!=null) {
			name.setText(gettitle(name.getContext(),uri,kind));
			}
		}
	}

static public void EnableControls(View view,boolean enable){
	view.setEnabled(enable);
	if (view instanceof ViewGroup) {
		ViewGroup vg = (ViewGroup) view;
		for (int i = 0; i < vg.getChildCount(); i++) {
			EnableControls(vg.getChildAt(i), enable);
		}
	}
}
 public void mkviews(MainActivity context,String label,View parview) {
 		if(parview!=null) {
//			parview.setVisibility(GONE);
			EnableControls(parview,false);
//				parview.setEnabled(false);
			}
		Button Select=getbutton(context,R.string.select);
		 name=getlabel(context,gettitle(context,uri,kind));
		final int rand=Math.round(5*GlucoseCurve.metrics.density);
		name.setPadding(rand,0,rand,0);
		CheckBox def=new CheckBox(context);
		def.setText(R.string.defaultname);
		final int minheight=GlucoseCurve.dpToPx(48);
		def.setMinimumHeight(minheight);
		def.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
				 if (isChecked) {
				 	uri=null;
					 name.setText(gettitle(context,uri,kind));
					 Select.setVisibility(INVISIBLE);
				 } else {
					 Select.setVisibility(VISIBLE);
				 }
			 }
			 );
			 	

		if(uri==null||uri.length()==0)
			def.setChecked(true);

		Select.setOnClickListener(v-> {
		       hidekeyboard(context);
		      try {
			    Intent intent = new Intent(RingtoneManager.ACTION_RINGTONE_PICKER);
			    intent.putExtra(RingtoneManager.EXTRA_RINGTONE_TYPE, kind==2?RingtoneManager.TYPE_NOTIFICATION:RingtoneManager.TYPE_ALARM);
			    intent.putExtra(RingtoneManager.EXTRA_RINGTONE_SHOW_SILENT, false);
			    intent.putExtra(RingtoneManager.EXTRA_RINGTONE_SHOW_DEFAULT, false);
			    final int request= MainActivity.REQUEST_RINGTONE|kind;
			    context.startActivityForResult(intent, request);
			    }
		    catch(Throwable th) {
			Toast.makeText(context, "No Ringtone Picker found", Toast.LENGTH_LONG).show();
		    	}
		});
		Button help=getbutton(context,R.string.helpname);
		help.setOnClickListener(v-> {
			help(R.string.ringtone,context);
			});
		EditText duredit=new EditText(context);
		duredit.setImeOptions(editoptions);
		duredit.setMinEms(3);
	        duredit.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
		duredit.setText(duration+"");
		duredit.setMinimumHeight(minheight);
	 TextView waitlabel=getlabel(context,R.string.minuteddeactivated);
		waitlabel.setPadding(rand*2,0,0,0);
	 flashview=new CheckBox(context);
	 permission=new Button(context);

	final boolean hasflash= !isWearable && Flash.hasFlash(context);
	CheckBox sound=new CheckBox(context);
	sound.setText(R.string.soundname);
	final boolean hassound= Natives.alarmhassound(kind);
	sound.setChecked(hassound);

	CheckBox vibration=new CheckBox(context);
	vibration.setText(R.string.vibrationname);
	final boolean hasvibration= Natives.alarmhasvibration(kind);
	vibration.setChecked(hasvibration);

	CheckBox disturb=new CheckBox(context);
	if(!isWearable) {

		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
			disturb.setText(R.string.disturb);
			final boolean dodisturb=getalarmdisturb(kind);
			disturb.setChecked(dodisturb);
			disturb.setOnCheckedChangeListener((buttonView,  isChecked) -> {
				 if (isChecked) {
					context.asknotificationAccess(); }
					}
					);
			}
		}
	 EditText waitedit=new EditText(context);
		waitedit.setMinimumHeight(minheight);
		Button play=getbutton(context,R.string.play);
		play.setOnClickListener(v-> {
			String str=duredit.getText().toString();
			try {
				hidekeyboard(context);
				int dur=(str!=null)?Integer.parseInt(str):duration;
		 		Ringtone ringtone = Notify.mkrings(uri, kind);
				Notify.playring(ringtone,dur,sound.isChecked(),hasflash&&flashview.isChecked(),vibration.isChecked(),isWearable||Build.VERSION.SDK_INT < Build.VERSION_CODES.M||disturb.isChecked(),kind);
				} catch(Throwable e) {
				Log.stack(LOG_ID,"play",e);
					Toast.makeText(context, "Can't play "+str+ " seconds", Toast.LENGTH_SHORT).show();
				
				}
			});

		Button Cancel=getbutton(context,R.string.cancel);
		Button Save=getbutton(context,R.string.save);


	var soundselect=isWearable||Build.VERSION.SDK_INT <Build.VERSION_CODES.M?new View[] {def,Select}:new View[] {def,Select,disturb};
	if(!hassound) {
		for(var el:soundselect)
			el.setEnabled(false);
		}
	sound.setOnCheckedChangeListener(
		 (buttonView,  isChecked) -> {
			 for (var el : soundselect)
				 el.setEnabled(isChecked);
		 }
		 );
		
	int hasname=(label==null&&kind>1)?0:1;
	int start=0;
	 View [][] views=new View[(kind<2?5:((label==null)?3:4))+3][];
View[] durviews;
	if(isWearable) {
		TextView durlabel=getlabel(context,R.string.duractionsec);
		durlabel.setPadding(rand*2,0,0,0);
		durviews=new View[]{durlabel,duredit};
		}
	else {
		TextView durlabel=getlabel(context,R.string.duraction);
		durlabel.setPadding(rand,0,0,0);
		TextView durseconds=getlabel(context,R.string.sec);
		 durviews=new View[]{help,durlabel,duredit,durseconds};
		}
//	View[] durviews=new View[]{help,durlabel,duredit,durseconds};
	
	views[1]=durviews;

	 if(kind<2) {
	 	start=2;
		 waitedit.setImeOptions(editoptions);
		 waitedit.setMinEms(3);
		 waitedit.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
		 waitedit.setText(susp+"");
		views[2]=new View[]{waitlabel,waitedit};
//		  views= new View[][]{new View[]{descript},new View[] {def,name,Select},durviews,new View[]{waitlabel,waitedit},new View[]{play,Cancel,Save}};
		
	 }
	 else start=1;

	views[0]=new View[]{name};
	views[start+1]=soundselect;
	if(hasname==1) {
		View labv=getlabel(context,label);
		views[views.length-1]=new View[]{labv};
		labv.setPadding(0,0,0,0);
		}
	views[views.length-1-hasname]= isWearable?new View[]{Cancel,Save}:new View[]{play,Cancel,Save};

	if(hasflash) {
		boolean flashalarm= Natives.alarmhasflash(kind);
		flashview.setChecked(flashalarm);
		flashview.setText(R.string.flash);
		flashview.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
			      var flashperm=Applic.hasPermissions( context, MainActivity.flashpermissions).length==0;
				permission.setVisibility(isChecked&&!flashperm?VISIBLE:INVISIBLE);
				 } 
				 );

		var flashperm2=Applic.hasPermissions( context, MainActivity.flashpermissions).length==0;
		permission.setText(R.string.permission);
		permission.setVisibility((!flashperm2&&flashalarm)?VISIBLE:INVISIBLE);

		permission.setOnClickListener(v->{
				var ret=context.flashpermission();
				if(ret==1)
					permission.setVisibility(INVISIBLE);
				 } 
				 );
	//	int pad=(int)(GlucoseCurve.metrics.density*4.0f);
//		permission.setPadding(0,0,pad,0);
	}
	else {
		flashview.setVisibility(INVISIBLE);
		permission.setVisibility(INVISIBLE);
		}
	views[start+2]=isWearable?new View[]{sound,vibration}:new View[]{sound,vibration};
	views[start+3]=isWearable?new View[]{play}:new View[]{flashview,permission};
	
	View lay;
	ScrollView scroll=new ScrollView(context);
	lay=scroll;
		Layout layout = new Layout(context, (l, w, h) -> {
		if(!isWearable) {
			final var width=GlucoseCurve.getwidth();
			if(width>w) {
				lay.setX((width-w)/2);
				}
			}
			//final var height=GlucoseCurve.getheight();
//			else ret=new int[]{width,height};
			return new int[]{w,h};}, views);
		scroll.addView(layout);
//		lay.setBackgroundColor(tk.glucodata.Applic.backgroundcolor);
	if(isWearable)  {
		lay.setBackgroundColor(tk.glucodata.Applic.backgroundcolor);
		 int laypad=(int)(GlucoseCurve.metrics.density*40.0f);
		 layout.setPadding(0,laypad,0,laypad);
		 }
	else {
		lay.setBackgroundResource(R.drawable.dialogbackground);
		 int laypad=(int)(GlucoseCurve.metrics.density*4.0f);
		 lay.setPadding(laypad,0,laypad,laypad);
		 }
		 /*
	if(isWearable)  {
		ScrollView scroll=new ScrollView(context);
		scroll.addView(layout);
		lay=scroll;
		lay.setBackgroundColor(tk.glucodata.Applic.backgroundcolor);
		 int laypad=(int)(GlucoseCurve.metrics.density*40.0f);
		 layout.setPadding(0,laypad,0,laypad);
		}
	else {
		lay=layout;
		lay.setBackgroundResource(R.drawable.dialogbackground);
		 int laypad=(int)(GlucoseCurve.metrics.density*4.0f);
		 lay.setPadding(laypad,0,laypad,laypad);
		}
		*/

	 context.addContentView(lay, isWearable?new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT):new ViewGroup.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));
//	 context.addContentView(lay, isWearable?new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT):new ViewGroup.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));
		Save.setOnClickListener(v->{
			Notify.stopalarm();
			try {
			String str=duredit.getText().toString();
			if(str!=null) {
				short durs=Short.parseShort(str);
				Natives.writealarmduration(kind,durs);
				}
			if(kind<2) {
				str = waitedit.getText().toString();
				if (str != null) {
					short wa = Short.parseShort(str);
					tk.glucodata.SuperGattCallback.writealarmsuspension(kind, wa);
				    }
			   }

			if(def.isChecked())
				uri="";
			  if(!Natives.writering(kind,uri,sound.isChecked(),hasflash&&flashview.isChecked(),vibration.isChecked())) {
				Toast.makeText(context, uri+" too large", Toast.LENGTH_SHORT).show();
				return;
			  	}
			//   Notify.setring(kind);
			} catch(Throwable e) {
				Log.stack(LOG_ID,"save",e);
				Toast.makeText(context, "Can't use specification", Toast.LENGTH_SHORT).show();
				return;
				};

			if(!isWearable)  {
				if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
					Natives.setalarmdisturb(kind,disturb.isChecked());
					}
				}

			if(parview!=null)
				EnableControls(parview,true);
			//	parview.setEnabled(true);

			//	parview.setVisibility(VISIBLE);

		        one=null;
		        hidekeyboard(context);
			lay.setVisibility(GONE);
			Settings.removeContentView(lay) ;
			context.poponback();
			});
		Cancel.setOnClickListener(v->{
			context.doonback();
			});
		context.setonback(() -> {
			Notify.stopalarm();
			if(parview!=null)
				EnableControls(parview,true);
				//parview.setVisibility(VISIBLE);
			one=null;
		        hidekeyboard(context);
			lay.setVisibility(GONE);
			Settings.removeContentView(lay);
			Notify.stopalarm();
			});
		}
}
