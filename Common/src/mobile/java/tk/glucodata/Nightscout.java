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

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Backup.getedit;
import static tk.glucodata.Backup.getnumedit;
import static tk.glucodata.MainActivity.CHAIN_REQUEST;
import static tk.glucodata.MainActivity.PRIVATE_REQUEST;
import static tk.glucodata.Natives.getreceiveport;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.provider.DocumentsContract;
import android.text.InputType;
import android.text.method.PasswordTransformationMethod;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Toast;

import java.io.File;

import tk.glucodata.settings.LibreNumbers;

public class Nightscout {
static final private String LOG_ID="Nightscout";
static private void openfile(MainActivity act,int requestid) {
	try {
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
		intent.addCategory(Intent.CATEGORY_OPENABLE);
		String name= MainActivity.keys[requestid&~PRIVATE_REQUEST];
		intent.setType("*/*");
        	intent.putExtra(Intent.EXTRA_TITLE, name);
//		var uri= Uri.fromFile(new File("/sdcard/Download",name));
//		 intent.putExtra(DocumentsContract.EXTRA_INFO,name);
//	    intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI, uri);

		act.startActivityForResult(intent, requestid);
	}
	catch(Throwable th) {
		Log.stack(LOG_ID,"openfile",th);
		}
    }

static final private int MAXKEY=80;
public static void show(MainActivity context,View parent) {
   	EnableControls(parent,false);

	var save=getbutton(context,R.string.save);
//	var visible=getcheckbox(context,"Visible",false);
	var secret=getlabel(context,R.string.secret);

	String key=Natives.getApiSecret();
	String[] oldkey={key};
//	var editkey=getedit(context,key);
    var editkey= new EditText(context);
        editkey.setImeOptions(editoptions);
        editkey.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
        editkey.setTransformationMethod(new PasswordTransformationMethod());
	 editkey.setMinEms(12);
	editkey.setText(key);

       var visible = new CheckBox(context);
       visible.setText(R.string.visible);
        visible.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
                        editkey.setInputType(isChecked?InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD:InputType.TYPE_TEXT_VARIATION_PASSWORD);
                        if(isChecked)
                                        editkey.setTransformationMethod(null);
                        else
                                        editkey.setTransformationMethod(new PasswordTransformationMethod());
                        });
	 var labport=getlabel(context,"SSL "+context.getString(R.string.port));
	var oldport=Natives.getsslport();
  	var portview=getnumedit(context, ""+oldport);
                        
   
	save.setOnClickListener(
		v -> {
		 var newkey=editkey.getText().toString();
		 if(newkey.length()>=MAXKEY) {
			Toast.makeText(context,newkey + context.getString(R.string.toolongsecret)+MAXKEY, Toast.LENGTH_LONG).show();
			return;
		 	}
		 var portstr=portview.getText().toString();
		 int portnum=0;
		 try {
                        portnum=Integer.parseInt(portstr);
                        }
                catch(Throwable e) {
                        Log.stack(LOG_ID,"parseInt", e);
			Toast.makeText(context,portstr+context.getString(R.string.invalidport), Toast.LENGTH_LONG).show();
			return;
                        };
		if(portstr.equals(getreceiveport())) {
			Toast.makeText(context,"The port number should be different from the mirror port",Toast.LENGTH_LONG).show();
			return;
			}
		if(portnum==17580) {
			Toast.makeText(context,"The port number should be different from the http port",Toast.LENGTH_LONG).show();
			return;
			}	
		if(portnum<1024||portnum> 65535) {
			Toast.makeText(context,R.string.portrange,Toast.LENGTH_LONG).show();
			return;
			}
		 if(!newkey.equals(oldkey[0])) {
		 	oldkey[0]=newkey;
			Toast.makeText(context,context.getString(R.string.newsecret)+newkey, Toast.LENGTH_LONG).show();
		 	Natives.setApiSecret(newkey);
			}
		if(portnum!= Natives.getsslport()) {
			Natives.setsslport(portnum);
			Toast.makeText(context,context.getString(R.string.newport)+portstr, Toast.LENGTH_LONG).show();
			if(Natives.getuseSSL())
				Natives.setuseSSL(true);
			}
		tk.glucodata.help.hidekeyboard(context);
		});
	var chain=getbutton(context,R.string.fullchain);
	chain.setOnClickListener(
		v -> {
			openfile(context,CHAIN_REQUEST);
		});

	var local=getcheckbox(context,R.string.localonly,Natives.getXdripServerLocal( ));
	float density=GlucoseCurve.metrics.density;
	int laypad=(int)(density*4.0);
	var httpport=getlabel(context,"http port=17580");
	httpport.setPadding(laypad*3,0,0,0);
	local.setOnCheckedChangeListener(
			 (buttonView,  isChecked) -> {
				Natives.setXdripServerLocal(isChecked);
			 });

	var privkey=getbutton(context,R.string.privatekey);
	privkey.setOnClickListener(
		v -> {
			openfile(context,PRIVATE_REQUEST);
		});
	var Help=getbutton(context,R.string.helpname);
	Help.setOnClickListener(
		v-> {
		help.help(R.string.Nightscouthelp,context);
		});
		

	var Close=getbutton(context,R.string.closename);

	var usexdripserver=Natives.getusexdripwebserver();
	var server=getcheckbox(context,R.string.active,usexdripserver);
	server.setOnCheckedChangeListener((buttonView, isChecked)-> {
		  Natives.setusexdripwebserver(isChecked);
		  });
	boolean usessl=Natives.getuseSSL();
	var sslbox=getcheckbox(context,R.string.usessl,usessl);
	boolean[] enabled={true};
	sslbox.setOnCheckedChangeListener((buttonView, isChecked)-> {
		if(enabled[0]) {
			String res=Natives.setuseSSL(isChecked);
			if(res!=null) {
				enabled[0]=false;
				sslbox.setChecked(!isChecked);
				enabled[0]=true;
				Toast.makeText(context, res, Toast.LENGTH_LONG).show();
				}
			else {
				if(isChecked)
					server.setChecked(true);
				}
			}
		});
	boolean saytreatments=Natives.getsaytreatments();
	var treatments=getcheckbox(context,R.string.treatments,saytreatments);
	int[] nochangeamounts={0};

	var layout=new Layout(context,(l,w,h)-> {
		var width= GlucoseCurve.getwidth();
		if(width>w)
			l.setX((width-w)/2);
		l.setY(0);
		return new int[] {w,h};
		},new View[]{secret,editkey},new View[]{visible,labport,portview,save} , new View[]{sslbox,privkey,chain},new View[]{local,httpport,treatments},new View[]{Help,server,Close} );
	treatments.setOnCheckedChangeListener( (buttonView,  isChecked) -> {
		switch(nochangeamounts[0])  {
			case 0: {
				++nochangeamounts[0];
				treatments.setChecked(!isChecked);
				LibreNumbers.mklayout(context,1,treatments,nochangeamounts,layout);
				};break;
			case  2: Natives.setsaytreatments(isChecked);break;

			};
		});
	layout.setPadding(laypad*2,laypad,laypad*2,laypad);

	layout.setBackgroundResource(R.drawable.dialogbackground);
	context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
	Runnable[] closeproc={null};

		closeproc[0]=()-> {
		 var newkey=editkey.getText().toString();
		context.setonback(closeproc[0]);
		 Runnable okproc= () -> {
                        context.poponback();
			EnableControls(parent,true);
			hidekeyboard(context);
			removeContentView(layout); 
			};
		 if(newkey.equals(oldkey[0])) {
			 var portstr=portview.getText().toString();
			 int portnum=0;
			 try {
				portnum=Integer.parseInt(portstr);
				}
			catch(Throwable e) {
				Log.stack(LOG_ID,"parseInt", e);
				};
			if(portnum== Natives.getsslport()) {
				okproc.run();
				return;
				}
			}
		Confirm.ask(context,context.getString(R.string.withoutsaving),"",okproc);
		};

	context.setonback( closeproc[0]);
	Close.setOnClickListener(
		v -> {
		        //  context.poponback();
                        context.doonback();

			});




		};
};
