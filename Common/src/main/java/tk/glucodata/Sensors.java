package tk.glucodata;

import static android.text.Html.TO_HTML_PARAGRAPH_LINES_CONSECUTIVE;
import static android.text.Html.fromHtml;
import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Layout.getMargins;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;

import android.graphics.Color;
import android.os.Build;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.ScrollView;
import android.widget.TextView;

class Sensors {
ViewGroup viewgroup;
CheckBox hide;
Button calview;
TextView textview;
long sensorptr;
void setSensorptr(long sensorptr) {
    this.sensorptr=sensorptr;
     boolean hasCali=Natives.calibrateNR( sensorptr,0)>0||Natives.calibrateNR( sensorptr,1)>0;

     if(hasCali) {
          calview.setVisibility(VISIBLE);
          }
     else {
           calview.setVisibility(GONE);
        }
    hide.setChecked(Natives.getHidefromSensorptr(sensorptr));
    }
void setSensorptrText(long sensorptr) {
   setSensorptr( sensorptr);
    settext(Natives.sensortextfromSensorptr(sensorptr));
    }
void settext(String text) {
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            textview.setText(fromHtml(text,TO_HTML_PARAGRAPH_LINES_CONSECUTIVE));
            }
        else {
            textview.setText(fromHtml(text));
            }
         }
Sensors(MainActivity act) {
         textview=new TextView(act);
         textview.setTextColor(Color.WHITE);
         textview.setTextIsSelectable(true);
         var close=getbutton(act,R.string.closename);
         calview=getbutton(act,R.string.calibrations);
         hide=getcheckbox(act,R.string.hide,false);
       hide.setOnCheckedChangeListener( (buttonView,  isChecked)-> {
                Natives.setHidefromSensorptr( sensorptr,isChecked);
                act.requestRender();
                });
         final View[][] views=
             isWearable? 
                new View[][]{new View[]{ textview},new View[]{hide},new View[]{calview},new View[]{close}}:
              new View[][]{new View[]{ textview},new View[]{hide,close},new View[]{calview}};
         var layout=new Layout(act, (l, w, h) -> {
            final int[] ret={w,h};
            return ret;
         }, views);
        var scroll=new ScrollView(act);
        scroll.addView(layout);
        scroll.setFillViewport(true);
        scroll.setSmoothScrollingEnabled(false);
       scroll.setScrollbarFadingEnabled(true);
       scroll.setVerticalScrollBarEnabled(Applic.scrollbar);
        if(!isWearable) {
            layout.setPadding((int)(GlucoseCurve.metrics.density*10),0,(int)(GlucoseCurve.metrics.density*5),0);
            }
         else {

            var height=GlucoseCurve.getheight();
                if(false) {
                //if(hasCali) 
                        int padside=(int)(GlucoseCurve.metrics.density*4);
                        layout.setPadding(padside,(int)(height*.1f),padside,(int)(height*.4f));
                 //       getMargins(close).rightMargin=(int)(width*.10f);
                  //      getMargins(calview).leftMargin=(int)(width*.08f);
                        }
                  else {
                        layout.setPadding(0,(int)(height*.1f),0,(int)(height*.01f));
                        }
                }
        close.setOnClickListener(v -> {
            MainActivity.doonback();
            });
        calview.setOnClickListener(v -> { CalibrateList.show(act,sensorptr,scroll); });
        viewgroup=scroll; 
        }
private static boolean isVisible=false;
static    void show(MainActivity act,String text, long sensorptr) {
    if(isVisible)
        return;
    isVisible=true;
    final var sensors=new Sensors(act);
    sensors.settext(text);
    sensors.setSensorptr(sensorptr);
    int param=isWearable?MATCH_PARENT:WRAP_CONTENT;
    var scroll=sensors.viewgroup;
    if(!isWearable) {
        scroll.setBackgroundResource(R.drawable.dialogbackground);
        scroll.measure(WRAP_CONTENT, WRAP_CONTENT);
        var width=GlucoseCurve.getwidth();
        scroll.setX((width-scroll.getMeasuredWidth()+MainActivity.systembarLeft-MainActivity.systembarRight)*.5f);
        }
    else {
        scroll.setBackgroundColor(Applic.backgroundcolor);
        }
    act.addContentView(sensors.viewgroup, new ViewGroup.LayoutParams(param,param));
    MainActivity.setonback(() -> {
            removeContentView(sensors.viewgroup);
            isVisible=false;
            });
     }

}
