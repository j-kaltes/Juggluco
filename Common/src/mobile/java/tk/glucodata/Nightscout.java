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
import static tk.glucodata.MainActivity.doonback;
import static tk.glucodata.MainActivity.poponback;
import static tk.glucodata.MainActivity.setonback;
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
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;

import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Toast;

import java.io.File;

import tk.glucodata.settings.LibreNumbers;

public class Nightscout {
static final private String LOG_ID="Nightscout";
static private void openfile(Activity act,int requestid) {
	try {
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
		intent.addCategory(Intent.CATEGORY_OPENABLE);
		String name= MainActivity.keys[requestid&~PRIVATE_REQUEST];
		intent.setType("*/*");
        	intent.putExtra(Intent.EXTRA_TITLE, name);

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
	var secret=getlabel(context,R.string.secret);

	String key=Natives.getApiSecret();
	String[] oldkey={key};
    var editkey= new EditText(context);
        editkey.setImeOptions(editoptions);
        editkey.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
        editkey.setTransformationMethod(new PasswordTransformationMethod());
	 editkey.setMinEms(12);
	editkey.setText(key);

       var visible = new CheckDirectionBox(context);
       //visible.setText(R.string.visible);
       visible.setButtonDrawable(R.drawable.password_visible);
/*      visible.setMinimumWidth(0); visible.setMinWidth(0);*/
        visible.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
                        var sel=editkey.getSelectionStart();
                        editkey.setInputType(isChecked?InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD:InputType.TYPE_TEXT_VARIATION_PASSWORD);
                        if(isChecked)
                                        editkey.setTransformationMethod(null);
                        else
                                        editkey.setTransformationMethod(new PasswordTransformationMethod());
                        editkey.setSelection(sel);
                        });
	 var labport=getlabel(context,"SSL "+context.getString(R.string.port));
	var oldport=Natives.getsslport();
  	var portview=getnumedit(context, ""+oldport);

	 var labhttpport=getlabel(context,"HTTP "+context.getString(R.string.port));
	int oldhttpport=Natives.gethttpport();
  	var httpportview=getnumedit(context, ""+oldhttpport);
                        
	 var labinterval=getlabel(context,R.string.interval);
	int interval=Natives.getinterval();
  	var intervalview=getnumedit(context, ""+interval);
   
	save.setOnClickListener(
		v -> {
		 var newkey=editkey.getText().toString();
		 if(newkey.length()>=MAXKEY) {
			Applic.argToaster(context,newkey + context.getString(R.string.toolongsecret)+MAXKEY, Toast.LENGTH_LONG);
			return;
		 	}
		 var portstr=portview.getText().toString();
		 int portnum=0;
		 try {
                        portnum=Integer.parseInt(portstr);
                        }
                catch(Throwable e) {
                        Log.stack(LOG_ID,"parseInt", e);
			Applic.argToaster(context,portstr+context.getString(R.string.invalidport), Toast.LENGTH_LONG);
			return;
                        };

		 var httpportstr=httpportview.getText().toString();
		 int httpportnum=0;
		 try {
                        httpportnum=Integer.parseInt(httpportstr);
                        }
                catch(Throwable e) {
                        Log.stack(LOG_ID,"parseInt HTTP", e);
			Applic.argToaster(context,httpportstr+context.getString(R.string.invalidport), Toast.LENGTH_LONG);
			return;
                        };
		if(portstr.equals(getreceiveport())||httpportstr.equals(getreceiveport())) {
			Applic.argToaster(context,R.string.nomirrorport,Toast.LENGTH_LONG);
			return;
			}
		if(portnum==httpportnum) {
			Applic.argToaster(context,R.string.nohttpport,Toast.LENGTH_LONG);
			return;
			}	
		if(portnum<1024||portnum>65535||httpportnum<1024||httpportnum>65535) {
			Applic.argToaster(context,R.string.portrange,Toast.LENGTH_LONG);
			return;
			}
		 if(!newkey.equals(oldkey[0])) {
		 	oldkey[0]=newkey;
			Applic.argToaster(context,context.getString(R.string.newsecret)+newkey, Toast.LENGTH_LONG);
		 	Natives.setApiSecret(newkey);
			}
		if(portnum!= Natives.getsslport()) {
			Natives.setsslport(portnum);
			Applic.argToaster(context,context.getString(R.string.newport)+portstr, Toast.LENGTH_LONG);
			if(Natives.getuseSSL())
				Natives.setuseSSL(true);
			}

		boolean warnhttpport=false;
		if(httpportnum!=Natives.gethttpport()) {
			boolean serveractive=Natives.getusexdripwebserver();
			Natives.sethttpport(httpportnum);
			Applic.argToaster(context,context.getString(R.string.newport)+httpportstr, Toast.LENGTH_LONG);
			if(serveractive) {
				Natives.setusexdripwebserver(false);
                util.sleep(1000);
				Natives.setusexdripwebserver(true);
				}
			warnhttpport=httpportnum!=17580;
			}

		 var intervalstr=intervalview.getText().toString();
		 int intervalnum=0;
		 try {
                        intervalnum=Integer.parseInt(intervalstr);
                        }
                catch(Throwable e) {
                        Log.stack(LOG_ID,"parseInt", e);
			Applic.argToaster(context,intervalstr+" invalid", Toast.LENGTH_LONG);
			return;
                        };
		Natives.setinterval(intervalnum);
		tk.glucodata.help.hidekeyboard(context);
		if(warnhttpport)
			Applic.argToaster(context,R.string.httpportxdripwarning,Toast.LENGTH_LONG);
		else
			Applic.argToaster(context, R.string.saved,Toast.LENGTH_SHORT);
		});
	var chain=getbutton(context,R.string.fullchain);
	chain.setOnClickListener(
		v -> {
			openfile(context,CHAIN_REQUEST);
		});

	var local=getcheckbox(context,R.string.localonly,Natives.getXdripServerLocal( ));
	float density=GlucoseCurve.metrics.density;
	int laypad=(int)(density*4.0);
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
           if(parent.getX()>0)
                help.helplight(R.string.Nightscouthelp, (MainActivity) context);
            else
                help.help(R.string.Nightscouthelp,context);
		});
		

	var UploadWeb=new android.widget.Button(context);
	UploadWeb.setText(R.string.webuploadpages);
	UploadWeb.setOnClickListener(v->WebPageUpload.show(context));

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
				Applic.argToaster(context, res, Toast.LENGTH_LONG);
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

	var errstr=Natives.nightError();
	var errorrow=errstr.length()>0?new View[]{getlabel(context,errstr)}:null;
	var layout=new Layout(context,(l,w,h)-> {
        /*
		var width= GlucoseCurve.getwidth();
		if(width>w)
			l.setX((width-w)/2);
		l.setY(MainActivity.systembarTop); */
		return new int[] {w,h};
		},new View[]{secret,editkey,visible},new View[]{labhttpport,httpportview,labport,portview},new View[]{local,labinterval,intervalview,save},new View[]{sslbox,privkey,chain},new View[]{treatments,UploadWeb},errorrow,new View[]{Help,server,Close} );
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

    var  params =    
            new FrameLayout.LayoutParams(
                    WRAP_CONTENT,
                    WRAP_CONTENT,
                    Gravity.CENTER_HORIZONTAL);

    context.addMyContentView(layout, params);
//	context.addMyContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
	Runnable[] closeproc={null};

        closeproc[0]=()-> {
                         var newkey=editkey.getText().toString();
                        setonback(closeproc[0]);
                         Runnable okproc= () -> {
                                poponback();
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
                                         var httpportstr=httpportview.getText().toString();
                                         int httpportnum=0;
                                         try {
                                                httpportnum=Integer.parseInt(httpportstr);
                                                }
                                        catch(Throwable e) {
                                                Log.stack(LOG_ID,"parseInt HTTP", e);
                                                };
                                         if(httpportnum==Natives.gethttpport()) {
                                                 var intervalstr=intervalview.getText().toString();
                                                 int intervalnum=0;
                                                 try {
                                                        intervalnum=Integer.parseInt(intervalstr);
                                                        }
                                                catch(Throwable e) {
                                                        Log.stack(LOG_ID,"parseInt", e);
                                                        };
                                                if(intervalnum==Natives.getinterval())  {
                                                        okproc.run();
                                                        return;
                                                        }
                                                }
                                        }
                                }
                        Confirm.ask(context,context.getString(R.string.withoutsaving),"",okproc);
                        };

	setonback( closeproc[0]);
	Close.setOnClickListener(
		v -> {
              doonback();

			});




		};
};
