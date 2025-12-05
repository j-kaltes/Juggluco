/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2, Libre 3, Dexcom G7/ONE+,              */
/*      Sibionics GS1Sb and Accu-Chek SmartGuide sensors.                            */
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
/*      Fri Nov 28 17:57:06 CET 2025                                                 */

package tk.glucodata;

import static android.view.View.INVISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Backup.getnumedit;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

import android.app.Activity;
import android.content.Context;
import android.text.InputType;
import android.text.method.PasswordTransformationMethod;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.Toast;



public class TurnServer {
static final private String LOG_ID="TurnServer";

/*
public static EditText getEditText(Context context, String key) {
    var editkey= new EditText(context);
    editkey.setImeOptions(editoptions);
    editkey.setInputType(InputType.TYPE_TEXT_VARIATION_PASSWORD);
    editkey.setTransformationMethod(new PasswordTransformationMethod());
    editkey.setMinEms(12);
    editkey.setText(key);
    return editkey;
    } */

static public  EditText getedit(Context act, String text) {
      EditText label=new EditText(act);
       label.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
       label.setImeOptions(editoptions);
       label.setMinEms(12);
      label.setText(text);
      return label;
      }
public static void show(Activity context,View parent) {
   EnableControls(parent,false);
   var delete=getbutton(context,context.getString(R.string.delete));
   var save=getbutton(context,R.string.save);
   var cancel=getbutton(context,R.string.cancel);
   var password = getlabel(context,R.string.password);
   var laypad=(int)(GlucoseCurve.getDensity()*4.0f);
   var absent=Natives.TurnServerNR()==0;
   password.setPadding(laypad*2,0,laypad,0);
   var username = getlabel(context,R.string.username);
   var hostname = getlabel(context,R.string.hostname);
   var portname = getlabel(context,R.string.port);
   var passedit=getedit(context,absent?"":Natives.getTurnPassword(0));
   var useredit=getedit(context,absent?"":Natives.getTurnUser(0));
   var hostedit=getedit(context,absent?"":Natives.getTurnHost(0));
  var portedit=getnumedit(context,absent?"":(""+Natives.getTurnPort(0)));

    delete.setOnClickListener(v->  {
        Natives.deleteTurnServer( 0);
        MainActivity.doonback();
        }
        );
   if(absent) {
        delete.setVisibility(INVISIBLE);
        }
    save.setOnClickListener(
            v -> {
             var portstr=portedit.getText().toString();
             int portnum=0;
             try {
                    portnum=Integer.parseInt(portstr);
                    }
            catch(Throwable e) {
                    Log.stack(LOG_ID,"parseInt", e);
                    Applic.argToaster(context,portstr+context.getString(R.string.invalidport), Toast.LENGTH_LONG);
                    return;
                    };
            if(portnum> 65535) {
                    Applic.argToaster(context,R.string.portrange,Toast.LENGTH_LONG);
                    return;
                    }
            Natives.setTurnPort(0,portnum);
            Natives.setTurnHost(0,hostedit.getText().toString());
            Natives.setTurnUser(0,useredit.getText().toString());
            Natives.setTurnPassword(0,passedit.getText().toString());
            MainActivity.doonback();
            }
            );
    var Help=getbutton(context,R.string.helpname);
    Help.setOnClickListener( v-> { 
            help.help(R.string.turnservers,context);
    });

    var layout=new Layout(context,(l,w,h)-> {
        return new int[] {w,h};
        },new View[]{hostname,hostedit,portname,portedit},new View[]{username,useredit,password,passedit} ,new View[]{cancel,Help,delete,save} );
    layout.setPadding(laypad*2,laypad,laypad*2,laypad);

    layout.setBackgroundResource(R.drawable.dialogbackground);
    context.addContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT, WRAP_CONTENT));
     MainActivity.setonback( () -> {
        EnableControls(parent,true);
        removeContentView(layout); 
        });
    cancel.setOnClickListener( v -> { MainActivity.doonback(); });
    }
};
