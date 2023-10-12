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

import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;

import java.util.ArrayList;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import tk.glucodata.Applic;
import tk.glucodata.Layout;
import tk.glucodata.Log;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;

import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static tk.glucodata.Natives.canSendNumbers;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.hideSystemUI;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;

public class LibreNumbers  {
private final static String LOG_ID="LibreNumbers";


static public class LibreNumberAdapter extends RecyclerView.Adapter<LibreNumberHolder> {
	ArrayList<String > 	labels=Natives.getLabels();

	ViewGroup layout;
	View sendnumbers;
	int night;

    LibreNumberAdapter(ViewGroup layout,View sendnumbers,int night) {
    	this.layout=layout;
    	this.sendnumbers=sendnumbers;
    	this.night=night;

    	}

    @NonNull
	@Override
    public LibreNumberHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    	Button view=new Button( parent.getContext());

	view.setTransformationMethod(null);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);
	view.setLayoutParams(new ViewGroup.LayoutParams(  ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        return new LibreNumberHolder(view,layout,sendnumbers,night);

    }

	@Override
	public void onBindViewHolder(final LibreNumberHolder holder, int pos) {
		TextView text=(TextView)holder.itemView;
		text.setText(labels.get(pos));
	    }
        @Override
        public int getItemCount() {
                return labels.size()-1;

        }

}





public static void    mklayout(MainActivity context,int night,CheckBox donum,int[] donothing,View parent) { 
	parent.setVisibility(INVISIBLE);
	Button close = new Button(context);
	close.setText(R.string.closename);
	RecyclerView recycle = new RecyclerView(context);
	recycle.setHasFixedSize(true);
	GridLayoutManager lin=new GridLayoutManager(context,3);
	recycle.setLayoutManager(lin);
	var help=getbutton(context,R.string.helpname);

//	var sendnumbers=getcheckbox(context,R.string.sendamounts,donum.isChecked());
	var sendnumbers=getcheckbox(context,donum.getText().toString(),donum.isChecked());
	if(!canSendNumbers(night)) {
		//EnableControls(sendnumbers,false);
		sendnumbers.setEnabled(false);
		}

	recycle.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
    final ViewGroup  librenumlayout=new Layout(context,(x,w,h)->{ hideSystemUI(); return new int[] {w,h}; },new View[] {recycle},new View[]{help,sendnumbers,close});
	var adapt = new LibreNumberAdapter(librenumlayout,sendnumbers,night);
	recycle.setAdapter(adapt);

	Runnable closerun=()-> {
		hidekeyboard(context);
		removeContentView(librenumlayout) ;
		final var checked= sendnumbers.isChecked();
	//	setSendNumbers(checked);
		parent.setVisibility(VISIBLE);
		donothing[0]=2;
		donum.setChecked(checked);
		donothing[0]=0;
		};
	context.setonback(closerun);
    close.setOnClickListener(v->{
	    context.poponback();
    	closerun.run();
    });

        librenumlayout.setBackgroundColor(Applic.backgroundcolor);
        context.addContentView(librenumlayout, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
	help.setOnClickListener(v->{
		EnableControls(librenumlayout,false);
		tk.glucodata.help.help(context.getString(night==1?R.string.nightnumhelp:R.string.librenumhelp),context,l-> {
			Log.i(LOG_ID,"librenumhelp callback");	
			EnableControls(librenumlayout,true); 
			if(!canSendNumbers(night)) { 
				sendnumbers.setEnabled(false);
				}
			} );

		});

	}

}
