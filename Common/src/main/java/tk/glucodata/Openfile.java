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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import java.io.File;
import java.io.FileInputStream;

import tk.glucodata.settings.Settings;

import static android.text.Html.TO_HTML_PARAGRAPH_LINES_CONSECUTIVE;
import static android.text.Html.fromHtml;
import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static tk.glucodata.Applic.backgroundcolor;
import static tk.glucodata.help.sethelpbutton;

class Openfile {
   final private String LOG_ID="Openfile";
   String libfile;
   File cali;
    String mess="";
//    ActivityResultLauncher<Intent> mStartForResult; //Dangerous, calls onActivityResult(int requestCode, int resultCode, Intent data)  with random requestCode's

//        mStartForResult = context.registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), new Downlib());
Layout lay=null;
   Openfile(String libfile,File cali) {
	this.cali=cali;
	this.libfile=libfile;
	}
	@SuppressLint("deprecation")
	void showchoice(MainActivity context,boolean getback) {
       
    if(lay!=null) {
	lay.setVisibility(VISIBLE);
	lay.bringToFront();
       Log.v(LOG_ID,"showchoice lay!=null");
    	return;
	}
    Log.v(LOG_ID,"showchoice "+mess);
    	help.hide();
	Button Open=new Button(context);Open.setText(R.string.open);
//	Button Urls=new Button(context);Urls.setText("Urls");
//	Urls.setOnClickListener(v-> { help.help(R.string.urls,context,l-> { l.setVisibility(GONE); }); });
//	Urls.setOnClickListener(v-> { help.help(R.string.urls,context); });
	sethelpbutton(VISIBLE);
	   TextView status=new TextView(context);status.setText(mess);
//	Button Retry=new Button(context);Retry.setText("Retry");
	Button Mirror=new Button(context);Mirror.setText(R.string.withoutsensor);


        TextView text=new TextView(context);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            text.setText(fromHtml(context.getString(R.string.getlib), TO_HTML_PARAGRAPH_LINES_CONSECUTIVE));
        }
        else
            text.setText(fromHtml(context.getString(R.string.getlib)));
           text.setTextIsSelectable(true);
	   text.setMovementMethod(LinkMovementMethod.getInstance());
 	    text.setLinksClickable(true);
	 lay=new Layout(context,new View[]{text},new View[]{Mirror,Open},new View[]{status});
	Open.setOnClickListener(v->{
//	    Settings.removeContentView(lay);
		lay.setVisibility(GONE);
		Settings.removeContentView(lay);
		lay=null;
		openlib(context);});
	lay.setBackgroundColor(backgroundcolor);
//	context.addContentView(lay, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
	context.addContentView(lay, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
	Mirror.setOnClickListener(v-> {
		lay.setVisibility(GONE);
		if(getback) {
			Natives.setusebluetooth(false);
   			help.help(R.string.introhelp,context);
			}
		else
			context.openfile=null;
//		help.show();
		});
	}
	/*
   class Downlib implements ActivityResultCallback<ActivityResult> {
        Downlib() {
        }
        @Override
        public void onActivityResult(ActivityResult res) {
            if (res.getResultCode() != Activity.RESULT_OK) {
		Log.i(LOG_ID,"onActivityResult on Ok");
                return;
		}
	Log.i(LOG_ID,"onActivityResult Ok");
            Intent intent = res.getData();
	    getlib(intent);
        }
    } */

static	boolean	reinstall=false;
 void   getlib(Intent intent,MainActivity context) {
		    if (intent == null) //Nodig?
			return;
		    Uri uri = intent.getData();
		    if (uri == null)//Nodig?
			return;
		Applic app= (Applic) context.getApplication();
		try {
		     FileInputStream inpu =(FileInputStream)app.getContentResolver().openInputStream(uri);
		  if(!getlibrary.unzipper(inpu, libfile, cali)) {
//			mess="Can't extract "+libfile+", try a different apk";
			   mess=String.format(context.getString(R.string.cantextract),libfile);

			}
		else {
			if(rightlib.testrightlib(context,cali)!=0)  {
				 Log.i(LOG_ID,"The right library");
				 if(context.openfile!=null) {
					if(context.openfile.lay!=null) {
						context.openfile.lay.setVisibility(GONE);
						Settings.removeContentView(context.openfile.lay);
						 context.openfile.lay=null;
						 }
					 context.openfile=null;
					 }
				if(reinstall) {
					reinstall=false;
					Natives.abbottreinit();
					}
				else  {
	//				help.whelplayout=null;
					help.help(R.string.introhelp,context);
					}
				 mess=context.getString(R.string.installedlibrary);
				}
			else {
				mess=context.getString(R.string.wronglibrary);
				}
			}

			Applic.Toaster(mess);
			Log.i(LOG_ID,mess);
			return;
	    } catch (Exception e) {
			Log.stack(LOG_ID,uri!=null?uri.toString():"",e);
			mess=e.getMessage();
			if(mess==null)
					mess="Exception";
			Applic.Toaster(mess);
	    }
    	}
    void openlib(Activity context) {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
       // mStartForResult.launch(intent);
       Log.v(LOG_ID,"openlib");
	context.startActivityForResult(intent,MainActivity.REQUEST_LIB);
    }
    }
