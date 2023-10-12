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

import android.app.Activity;
import android.content.Intent;
import android.text.InputType;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import static java.lang.System.currentTimeMillis;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Locale;

//import org.w3c.dom.Text;

public class Dialogs {
private	final static String LOG_ID="Dialogs";
private float density;
private ViewGroup exportscreen=null;
 TextView exportlabel=null;
Dialogs(float density) {
	this.density=density;
	}
private Button exportbutton(MainActivity activity,String label, int type) {
	Button but=new Button(activity);
	but.setText(label);
        but.setOnClickListener(
		v-> {
		if(type==4)
		    algexporter(activity,   type,label,".html");
		   else
		    exporter( activity,  type,label);
		    });
    return but;
	}


void showexport(MainActivity activity,int width,int height) {
	if(exportscreen==null) {
		Button num=exportbutton(activity,activity.getString(R.string.amountsname),0);
		Button scan=exportbutton(activity,activity.getString(R.string.scansname),1);
		Button stream=exportbutton(activity,activity.getString(R.string.streamname),2);
		Button hist=exportbutton(activity,activity.getString(R.string.historyname),3);
		Button meals=exportbutton(activity,activity.getString(R.string.mealsname),4);
		exportlabel=new TextView(activity);
//		exportlabel.setElegantTextHeight(true);
		exportlabel.setInputType(InputType.TYPE_TEXT_FLAG_MULTI_LINE);
		exportlabel.setSingleLine(false);
		final int rand=Math.round(5*density);
		exportlabel.setPadding(0,rand,0,rand);
		Button close=new Button(activity);
		close.setText(R.string.closename);
		close.setOnClickListener(v-> activity.doonback());
		View[] gviews={scan,hist,stream};
		View[] lviews={num,meals,close};
//		exportlabel.requestFocus();
		exportscreen=new Layout(activity, (lay, w, h) -> {
			if(w>=width||h>=height) {
				lay.setX(0);
				lay.setY(0);
				}
			else {
				lay.setX((width-w)/2); lay.setY((height-h)/2);
				}
			return new int[] {w,h};

		}, gviews,new View[]{exportlabel},lviews);
		exportscreen.setPadding(rand,rand,rand,rand);
		exportscreen.setBackgroundColor( Applic.backgroundcolor);
		activity.addContentView(exportscreen, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
//		exportlabel.requestFocus();
		}
	else {
        	exportscreen.setVisibility(VISIBLE);
		}
	exportlabel.setText(R.string.exporthelp);
	activity.setonback(() -> {
        		exportscreen.setVisibility(View.GONE);
			if(Menus.on)
				Menus.show(activity);
			}
			);
	}
static	public final DateFormat fdatename=             new SimpleDateFormat("yyyy-MM-dd-HH:mm:ss", Locale.US);
static void algexporter(MainActivity context,int type,String prefix,String ext) {
	final long time=currentTimeMillis();
	final String datestr=fdatename.format(time)      ;
        final String filename = prefix+datestr+ext;
        exportdata(context,type,filename);
	}
static void exporter(MainActivity context,int type,String prefix) {
        algexporter(context,type,prefix,".tsv");
	}


static private void exportdata(MainActivity 	context,int type,String name) {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        //intent.setType("text/tsv");
        intent.putExtra(Intent.EXTRA_TITLE, name);
	intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
//      ((ActivityResultLauncher<Intent>)exports[type]).launch(intent);
      	int request= MainActivity.REQUEST_EXPORT|type;
	try {
		context.startActivityForResult(intent, request);
		} catch(Throwable th) {

		Log.stack(LOG_ID,"ACTION_CREATE_DOCUMENT",th);
		}
    }


	}
