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
/*      Sun Apr 16 20:58:46 CEST 2023                                                 */


package tk.glucodata.settings;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.MainActivity.getscreenwidth;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.help;
import static tk.glucodata.settings.Settings.edit2float;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.hideSystemUI;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getlabel;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.InputType;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

import tk.glucodata.Applic;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.LabelAdapter;
import tk.glucodata.Layout;
import tk.glucodata.Log;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;

class LabelsClass {
	private static final String LOG_ID="LabelsClass";
final MainActivity activity;
LabelsClass(MainActivity context ) {
	activity=context;
	}

boolean garminwatch=Natives.gethasgarmin();;
ArrayList<String > labels;
    EditText label;
EditText labelprec;
EditText labelweight;
Button delete=null;
   int labelpos=-1;
LabelListAdapter    adapt;
void mkchangelabel(MainActivity context,Runnable onsave,View parent) {
    	EnableControls(parent,false);
        TextView labeltext = new TextView(context);
        labeltext.setText(R.string.numlabel);
        label = new EditText(context);
        label.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);

        label.setImeOptions(editoptions);
        label.setMinEms(7);
	View[] precview=null;
        if(garminwatch) {
            TextView prectext = new TextView(context);
            prectext.setText(R.string.roundto);
            labelprec = new EditText(context);
            labelprec.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
            labelprec.setMinEms(3);
	     labelprec.setImeOptions(editoptions);
	     precview=new View[]{prectext,labelprec};
            }
        TextView weighttext = new TextView(context);
        weighttext.setText(R.string.weight);
        labelweight = new EditText(context);
        labelweight.setInputType( InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        labelweight.setMinEms(3);
        labelweight.setImeOptions(editoptions);
        Button help=new Button(context);
        help.setText(R.string.helpname);
        help.setOnClickListener(v->{help(R.string.newlabelhelp,(MainActivity)(v.getContext()));});
        Button cancel=new Button(context);
        cancel.setText(R.string.cancel);
        cancel.setOnClickListener(v->{
		context.doonback();
		});
        Button save=new Button(context);
	View [][] views= garminwatch?(new View[][]{new View[]{labeltext,label}, precview,  new View[]{weighttext, labelweight} ,new View[]{help,cancel,save}}):(new View[][]{new View[]{labeltext,label},  new View[]{weighttext, labelweight} ,new View[]{help,cancel,save}});

        var editlabel = new Layout(context,
                (l, w, h) -> {
    		hideSystemUI();
			var width=getscreenwidth(context);
			if(width>w)
				  l.setX(( width- w) *.6f);
                    l.setY(0);
			return new int[] {w,h};
                }, views);
	if(Natives.staticnum()) {
		save.setVisibility(INVISIBLE);
		}
	else {
		save.setText(R.string.save);
		save.setOnClickListener(v-> {
			float pr=garminwatch?edit2float(labelprec):0;
			float wei=edit2float(labelweight);
			String name=label.getText().toString();
			int pos=(labelpos>=0) ? labelpos:labels.size()-1;

			if(!Natives.setlabel(pos,name,pr,wei)) {
				Applic.argToaster(context, name+context.getString(R.string.toolarg), Toast.LENGTH_SHORT);
				return;
				}
			if(labelpos>=0) {
			    labels.set(pos,name);
			    }
			else {
			    labels.add(pos,name);
			  if(delete!=null) {
				    delete.setVisibility(VISIBLE);
				    }

			    }
 			tk.glucodata.help.hidekeyboard(activity) ;

		       editlabel.setVisibility(GONE);
			adapt.notifyDataSetChanged();
    			EnableControls(parent,true);
			onsave.run();
			context.poponback();
			} );
		}
	      editlabel.setBackgroundResource(R.drawable.dialogbackground);
	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
	   editlabel.setPadding(pad,0,pad,0);
        context.addContentView(editlabel, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));



	context.setonback(() -> {
 		tk.glucodata.help.hidekeyboard(activity) ;
		editlabel.setVisibility(GONE);
		EnableControls(parent,true);
		if(editlabel!=null) removeContentView(editlabel) ;
		});
}

private void	dodeletelast(Spinner spinner,	LabelAdapter<String> numspinadapt,Button addnew, int nr) {
	Natives.setnrlabel(nr);
	Log.i(LOG_ID,"voor remove labels.size()="+labels.size());
	labels.remove(nr); //USE
	Log.i(LOG_ID,"na remove labels.size()="+labels.size());
	adapt.notifyDataSetChanged();
	numspinadapt.setarray(Natives.getLabels());
	spinner.setAdapter(numspinadapt);
	spinner.setSelection(Natives.getmealvar());

	if((labels.size()-1)<40) {
		Log.i(LOG_ID,"addnew.setVisibility(VISIBLE)");
		addnew.setVisibility(VISIBLE);
		}
	}

private void	askdeletelast(Spinner spinner,	LabelAdapter<String> numspinadapt, Button addnew,int nr) {

        AlertDialog.Builder builder = new AlertDialog.Builder(activity);
        builder.setTitle(R.string.deletelabel).
	 setMessage(labels.get(nr)).
        setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
		 		dodeletelast(spinner,numspinadapt,addnew,nr);
                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show().setCanceledOnTouchOutside(false);
	}

void    mklabellayout(View parent ) {
	parent.setVisibility(INVISIBLE);
	labels=Applic.app.getlabels();
	MainActivity context = activity;
//    if(labellayout==null) {
	Button ok = new Button(context);
	ok.setText(R.string.ok);
	RecyclerView recycle = new RecyclerView(context);
	recycle.setHasFixedSize(true);
	LinearLayoutManager lin = new LinearLayoutManager(context);
	recycle.setLayoutManager(lin);
	Button addnew = new Button(context);

        Spinner spinner=new Spinner(context);
	final int minheight= GlucoseCurve.dpToPx(48);
	spinner.setMinimumHeight(minheight);
	avoidSpinnerDropdownFocus(spinner);
	LabelAdapter<String> numspinadapt=new LabelAdapter<String>(context,Natives.getLabels(),1);
        spinner.setAdapter(numspinadapt);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
		@Override
		public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
			Natives.setmealvar((byte)position);
			}
		@Override
		public  void onNothingSelected (AdapterView<?> parent) {

		} });
	final Runnable onsave= ()->  {
				numspinadapt.setarray(Natives.getLabels());
				spinner.setAdapter(numspinadapt);
				spinner.setSelection(Natives.getmealvar());
				if((labels.size()-1)>=40)
					addnew.setVisibility(INVISIBLE);
				};

	delete = new Button(context);
	TextView menulabel=getlabel(context,context.getString(R.string.meal));
//	spinner.clearAnimation();
	spinner.setSelection(Natives.getmealvar());
        Button help=new Button(context);
        help.setOnClickListener(v->{help(R.string.labelhelp,context); });
        help.setText(R.string.helpname);
	Layout butlay=new Layout(context,new View[]{menulabel,spinner},new View[] {delete},new View[]{help},new View[]{addnew},new View[]{ok});
	butlay.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
		recycle.setLayoutParams(new ViewGroup.LayoutParams(   ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.MATCH_PARENT));
    final ViewGroup  labellayout=new Layout(context,(x,w,h)->{
    			hideSystemUI();
			return new int[] {w,h};
	},new View[] {recycle,butlay});

	adapt = new LabelListAdapter(labels, this,onsave,labellayout); //USE
	recycle.setAdapter(adapt);

	if (Natives.staticnum()) {
		addnew.setVisibility(INVISIBLE);
		delete.setVisibility(INVISIBLE);
	} else {
		addnew.setOnClickListener(v -> {
			mkchangelabel(activity,onsave,labellayout); //USE
			label.setText("");
			labelpos = -1;
		});
		addnew.setText(R.string.newname);
		delete.setText(R.string.deletelast);


		if((labels.size()-1)>=40)
			addnew.setVisibility(INVISIBLE);


		delete.setOnClickListener(v -> {
			int nr = labels.size() - 2; //USE
			Log.d(LOG_ID, "delete " + nr);
			if (nr >= 0) {
				askdeletelast(spinner,numspinadapt,addnew, nr);
				}
			if (nr <= 0)
				delete.setVisibility(INVISIBLE);
		});
	if (labels.size() < 2)  //USE
		delete.setVisibility(INVISIBLE);

	}

	Runnable closerun=()-> {
		parent.setVisibility(VISIBLE);
 		tk.glucodata.help.hidekeyboard(activity) ;
		Applic app=(tk.glucodata.Applic )context.getApplication();
		if(Natives.shouldsendlabels())  {
		    Applic.wakemirrors();
		    app.sendlabels();
		}
		/*
		if(app.curve!=null&&app.curve.search!=null) {
			app.curve.searchspinadap.setarray(Natives.getLabels());
			app.curve.searchspinner.setAdapter(app.curve.searchspinadap);
			} */
		removeContentView(labellayout) ;
		};
	context.setonback(closerun);
    ok.setOnClickListener(v->{
	    context.poponback();
    	closerun.run();
    });

        labellayout.setBackgroundColor(Applic.backgroundcolor);
        context.addContentView(labellayout, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
   /* }
    else {
        labellayout.setVisibility(VISIBLE); 
          labellayout.bringToFront();
	}*/
}
static public class LabelListAdapter extends RecyclerView.Adapter<LabelListHolder> {
    ArrayList<String> labels=null;
    LabelsClass settings;
    View parlayout;
    Runnable onsave;
    LabelListAdapter(ArrayList<String> labels,LabelsClass set,Runnable onsave,View parlayout) {
        this.labels=labels;
	this.parlayout=parlayout;
	this.onsave=onsave;
        settings=set;
    }

    @NonNull
	@Override
    public LabelListHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    	Button view=new Button( parent.getContext());

	view.setTransformationMethod(null);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);
	view.setLayoutParams(new ViewGroup.LayoutParams(  ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

        return new LabelListHolder(view,settings,onsave,this.parlayout);

    }

	@Override
	public void onBindViewHolder(final LabelListHolder holder, int pos) {
		TextView text=(TextView)holder.itemView;
		text.setText(labels.get(pos));
	    }
        @Override
        public int getItemCount() {
                return labels.size()-1;

        }

}
}
