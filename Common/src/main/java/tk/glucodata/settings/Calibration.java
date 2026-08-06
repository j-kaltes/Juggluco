
package tk.glucodata.settings;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.backgroundcolor;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Natives.setshowcalibratedstream;
import static tk.glucodata.Natives.setshowstream;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.RingTones.EnableControls;


import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.FrameLayout;
import android.widget.ScrollView;
import android.widget.Spinner;

import tk.glucodata.Applic;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.LabelAdapter;
import tk.glucodata.Layout;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;

public class Calibration  {


static public void show(MainActivity act,View parent) {
    if(parent!=null)  {
//           parent.setVisibility(GONE);
           EnableControls(parent,false);
//           if(!isWearable) act.lightBars(!Natives.getInvertColors());
            }
    Spinner spinner=new Spinner(act);
    avoidSpinnerDropdownFocus(spinner);
    LabelAdapter<String> numspinadapt=new LabelAdapter<String>(act, Natives.getLabels(),0);
    spinner.setAdapter(numspinadapt);
    spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
            Natives.setbloodvar((byte)position);
            }
        @Override
        public  void onNothingSelected (AdapterView<?> parent) {

        } });
   spinner.setSelection(Natives.getbloodvar());
   var bloodvar=getlabel(act,R.string.bloodvar);
    float density=GlucoseCurve.metrics.density;
   bloodvar.setPaddingRelative((int)(density*10),0,(int)(4*density),0);
   var close=getbutton(act, R.string.closename);
   var allvalues=getcheckbox(act,act.getString(R.string.allvalues),Natives.getAllValues());
   allvalues.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
          Natives.setAllValues(isChecked);
          act.requestRender();
          });
   var docalibrate=getcheckbox(act,act.getString(R.string.active),Natives.getDoCalibrate());
   docalibrate.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
          Natives.setDoCalibrate(isChecked);
          if(isChecked) {
            setshowcalibratedstream(true);
            }
          act.requestRender();
          });
   var calibratepast=getcheckbox(act,act.getString(R.string.calibratepast),Natives.getCalibratePast());
   calibratepast.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
          Natives.setCalibratePast(isChecked);
          act.requestRender();
          });
   var calibrateA=getcheckbox(act,act.getString(R.string.calibrate_a),Natives.getCalibrateA());
   calibrateA.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
          Natives.setCalibrateA(isChecked);
          });
  View[][] views;
  if(isWearable) {
        var buttons=new View[]{docalibrate,close};
        views=new View[][]{new View[]{calibrateA},
       new View[]{bloodvar,spinner},buttons,new View[]{calibratepast},new View[]{allvalues}} ;
    }
    else {
       var help=getbutton(act, R.string.helpname);
        help.setOnClickListener(v->{
             tk.glucodata.help.help(R.string.calibrationhelp,act);
            });
         var  buttons=new View[]{help,docalibrate,close};
         var lastrow=new View[]{allvalues,calibratepast} ;
        views=new View[][]{new View[]{calibrateA}, new View[]{bloodvar,spinner},buttons,     lastrow};
        };
    ViewGroup  layout=new Layout(act, (lay, w, h) -> { return new int[] {w,h};},views );
  if(isWearable) {
    layout.setBackgroundColor(backgroundcolor);
    var height=    GlucoseCurve.getheight();
//    layout.setPadding((int)(height*.02f),(int)(height*.11f), (int)(height*.05f), (int)(height*.11f));
    layout.setPaddingRelative((int)(height*.02f),(int)(height*.17f), (int)(height*.05f), 0);
    //Layout.getMargins(allvalues);
    //pasmarg.leftMargin=(int)(height*.07f);
    var pasmarg=Layout.getMargins(allvalues);
    pasmarg.bottomMargin=(int)(height*.14f);
        var scroll=new ScrollView(act);
        scroll.addView(layout);
        scroll.setFillViewport(true);
        scroll.setSmoothScrollingEnabled(false);
       scroll.setScrollbarFadingEnabled(true);
       scroll.setVerticalScrollBarEnabled(Applic.scrollbar);


    act.addMyContentView(scroll, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
    layout=scroll;

  }
  else {
     layout.setBackgroundResource(R.drawable.dialogbackground);
    var  params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER| Gravity.CENTER_HORIZONTAL);

    act.addMyContentView(layout, params);
    int pad=(int)(density*5.0f);
    layout.setPadding(pad,pad,pad,pad);

    }
    final ViewGroup flayout=layout;
    MainActivity.setonback( () -> {
      if(parent!=null)  {
          // parent.setVisibility(VISIBLE);

           EnableControls(parent,true);
//           if(!isWearable) act.themeLightBars();
            }
        removeContentView(flayout);
        });

    close.setOnClickListener(v->{
    	MainActivity.doonback();
    	});
   }


};
