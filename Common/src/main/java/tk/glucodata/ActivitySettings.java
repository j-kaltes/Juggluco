// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

import android.app.AlertDialog;
import android.app.DatePickerDialog;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

/** Activity dialogs use the same Layout and theme as the other Juggluco views. */
public final class ActivitySettings {
    private ActivitySettings() {}
    private static final int[] intervals={0,900,1800,3600,10800,21600,86400};
    private static String date(long seconds) {
        return DateFormat.getDateTimeInstance(DateFormat.SHORT,DateFormat.SHORT).format(new Date(seconds*1000));
    }
    private static void safely(MainActivity act,Runnable run) {
        try {run.run();} catch(Exception e){Toast.makeText(act,e.getMessage(),Toast.LENGTH_LONG).show();}
    }
    private static Spinner spinner(MainActivity act,String[] values) {
        Spinner view=tk.glucodata.settings.Settings.getGenSpin(act);
        view.setAdapter(new ArrayAdapter<>(act,android.R.layout.simple_spinner_dropdown_item,values));
        return view;
    }
    private static void present(MainActivity act,View parent,Layout layout) {
        if(parent!=null)RingTones.EnableControls(parent,false);
        layout.setBackgroundColor(Applic.backgroundcolor);
        int pad=GlucoseCurve.dpToPx(8);
        layout.systembarPadding((l,t,r,b)->new int[]{l+pad,t+pad,r+pad,b+pad});
        act.addMyContentView(layout,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
        MainActivity.setonback(()->{
            tk.glucodata.settings.Settings.removeContentView(layout);
            if(parent!=null)RingTones.EnableControls(parent,true);
            act.requestRender();
        });
    }
    public static void show(MainActivity act,View parent) {
        safely(act,()->{
            long[] config=ActivityNative.config();
            long[] from={config[0]==0?System.currentTimeMillis()/1000-7*86400:config[0]};
            TextView title=getlabel(act,R.string.activity_import);
            Button since=getbutton(act,date(from[0]));
            since.setOnClickListener(v->{Calendar cal=Calendar.getInstance();cal.setTimeInMillis(from[0]*1000);
                new DatePickerDialog(act,(picker,y,m,d)->{cal.set(y,m,d,0,0,0);cal.set(Calendar.MILLISECOND,0);
                    from[0]=cal.getTimeInMillis()/1000;since.setText(date(from[0]));},cal.get(Calendar.YEAR),cal.get(Calendar.MONTH),cal.get(Calendar.DAY_OF_MONTH)).show();
            });
            Spinner interval=spinner(act,act.getResources().getStringArray(R.array.activity_intervals));
            for(int i=0;i<intervals.length;++i)if(intervals[i]==config[1])interval.setSelection(i);
            TextView status=getlabel(act,"");
            Button retrieve=getbutton(act,R.string.activity_retrieve),labels=getbutton(act,R.string.activity_labels);
            Button permissions=getbutton(act,R.string.activity_permissions);
            Button list=getbutton(act,R.string.activity_saved),save=getbutton(act,R.string.save),close=getbutton(act,R.string.closename);
            Layout form=new Layout(act,new View[]{title},new View[]{getlabel(act,R.string.activity_from),since},
                new View[]{getlabel(act,R.string.activity_interval),interval},new View[]{retrieve,permissions},new View[]{labels,list},
                new View[]{getlabel(act,R.string.activity_access)},new View[]{status});
            ScrollView scroll=new ScrollView(act);scroll.addView(form,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));
            Layout layout=contentLayout(act,scroll,new View[]{save,close});
            Runnable apply=()->{long[] current=ActivityNative.config();ActivityNative.configure(from[0],intervals[interval.getSelectedItemPosition()],(int)current[3]);ActivityHealth.settingsChanged(act);};
            save.setOnClickListener(v->safely(act,()->{apply.run();MainActivity.doonback();}));
            retrieve.setOnClickListener(v->safely(act,()->{apply.run();ActivityHealth.retrieve(act);}));
            permissions.setOnClickListener(v->ActivityHealth.permissions(act));
            labels.setOnClickListener(v->safely(act,()->mappings(act,layout)));
            list.setOnClickListener(v->safely(act,()->activities(act,layout)));
            close.setOnClickListener(v->MainActivity.doonback());
            present(act,parent,layout);
            Handler handler=new Handler(Looper.getMainLooper());
            Runnable refresh=new Runnable(){public void run(){
                if(!layout.isAttachedToWindow())return;
                long last=ActivityNative.config()[2];
                status.setText(act.getString(R.string.activity_last)+" "+(last==0?act.getString(R.string.activity_never):date(last))+"\n"+ActivityHealth.status());
                handler.postDelayed(this,1000);
            }};handler.post(refresh);
        });
    }
    private static void mappings(MainActivity act,View parent) {
        int[] types=ActivityHealth.typeCodes();ArrayList<String> rows=new ArrayList<>();
        String[] measures=act.getResources().getStringArray(R.array.activity_measures);
        ArrayAdapter<String> adapter=new ArrayAdapter<>(act,android.R.layout.simple_list_item_1,rows);
        Runnable refresh=()->{
            ArrayList<String> labels=Natives.getLabels();rows.clear();
            int defaultLabel=(int)ActivityNative.config()[3];
            rows.add(act.getString(R.string.activity_default)+" → "+(defaultLabel>=0&&defaultLabel<labels.size()?labels.get(defaultLabel):"Activity"));
            for(int type:types) {int[] rule=ActivityNative.rule(type);String name=rule[0]>=0&&rule[0]<labels.size()?labels.get(rule[0]):act.getString(R.string.activity_default);
                rows.add(ActivityHealth.typeName(type)+" → "+name+" · "+measures[rule[1]]);}
            adapter.notifyDataSetChanged();
        };refresh.run();
        ListView list=new ListView(act);list.setAdapter(adapter);
        Button close=getbutton(act,R.string.closename);
        Layout layout=listLayout(act,list,close);
        list.setOnItemClickListener((a,v,pos,id)->mapping(act,layout,pos==0?-1:types[pos-1],refresh));
        close.setOnClickListener(v->MainActivity.doonback());present(act,parent,layout);
    }
    private static Layout listLayout(MainActivity act,ListView list,Button close) {
        return contentLayout(act,list,new View[]{close});
    }
    private static Layout contentLayout(MainActivity act,View content,View[] buttons) {
        return new Layout(act,new View[]{content},buttons) {
            @Override protected void onMeasure(int widthSpec,int heightSpec) {
                int rowHeight=0;
                for(View button:buttons) {
                    button.measure(MeasureSpec.makeMeasureSpec(MeasureSpec.getSize(widthSpec),MeasureSpec.AT_MOST),
                        MeasureSpec.makeMeasureSpec(0,MeasureSpec.UNSPECIFIED));
                    rowHeight=Math.max(rowHeight,button.getMeasuredHeight());
                }
                ViewGroup.LayoutParams params=content.getLayoutParams();
                params.width=Math.max(1,MeasureSpec.getSize(widthSpec)-getPaddingLeft()-getPaddingRight());
                params.height=Math.max(1,MeasureSpec.getSize(heightSpec)-getPaddingTop()-getPaddingBottom()-rowHeight);
                super.onMeasure(widthSpec,heightSpec);
            }
        };
    }
    private static void mapping(MainActivity act,View parent,int type,Runnable refreshed) {
        int[] rule=type<0?new int[]{(int)ActivityNative.config()[3],0}:ActivityNative.rule(type);
        ArrayList<String> names=new ArrayList<>();ArrayList<Integer> ids=new ArrayList<>();
        ArrayList<String> all=Natives.getLabels();
        for(int i=0;i<all.size();++i)if(ActivityNative.labelAllowed(i)){names.add(all.get(i));ids.add(i);}
        Spinner label=spinner(act,names.toArray(new String[0]));
        int selected=ids.indexOf(rule[0]);if(selected>=0)label.setSelection(selected);
        Spinner measure=spinner(act,act.getResources().getStringArray(R.array.activity_measures));measure.setSelection(rule[1]);
        CheckDirectionBox existing=util.getcheckbox(act,R.string.activity_update_existing,false);
        Button add=getbutton(act,R.string.activity_new_label),save=getbutton(act,R.string.save),close=getbutton(act,R.string.closename);
        Layout form=new Layout(act,new View[]{getlabel(act,type<0?act.getString(R.string.activity_default):ActivityHealth.typeName(type))},
            new View[]{label,add},type<0?null:new View[]{measure},type<0?null:new View[]{existing},type<0?null:new View[]{getlabel(act,R.string.activity_measure_future)});
        ScrollView scroll=new ScrollView(act);scroll.addView(form,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT));
        Layout layout=contentLayout(act,scroll,new View[]{save,close});
        add.setOnClickListener(v->{EditText text=new EditText(act);
            new AlertDialog.Builder(act).setTitle(R.string.activity_new_label).setView(text)
                .setPositiveButton(R.string.save,(dialog,which)->safely(act,()->{int id=ActivityNative.newLabel(text.getText().toString().trim());
                    names.add(Natives.getLabels().get(id));ids.add(id);label.setAdapter(new ArrayAdapter<>(act,android.R.layout.simple_spinner_dropdown_item,names));label.setSelection(ids.size()-1);
                })).setNegativeButton(R.string.cancel,null).show();
        });
        save.setOnClickListener(v->safely(act,()->{
            if(ids.isEmpty())throw new IllegalStateException(act.getString(R.string.activity_choose_label));
            int id=ids.get(label.getSelectedItemPosition());
            if(type<0){long[] cfg=ActivityNative.config();ActivityNative.configure(cfg[0],(int)cfg[1],id);}
            else ActivityNative.setRule(type,id,measure.getSelectedItemPosition(),existing.isChecked());
            if(refreshed!=null)refreshed.run();
            MainActivity.doonback();
        }));
        close.setOnClickListener(v->MainActivity.doonback());present(act,parent,layout);
    }
    private static void activities(MainActivity act,View parent) {
        int[] ids=ActivityNative.indices(false);ArrayList<String> rows=new ArrayList<>();
        for(int id:ids){double[] r=ActivityNative.record(id);String[] t=ActivityNative.texts(id);
            rows.add(date((long)r[3])+" · "+ActivityHealth.typeName((int)r[2])+(t[2].isEmpty()?"":" · "+t[2]));}
        if(ids.length==0){Toast.makeText(act,R.string.activity_none,Toast.LENGTH_LONG).show();return;}
        ListView list=new ListView(act);list.setAdapter(new ArrayAdapter<>(act,android.R.layout.simple_list_item_1,rows));
        Button close=getbutton(act,R.string.closename);Layout layout=listLayout(act,list,close);
        list.setOnItemClickListener((a,v,pos,id)->safely(act,()->details(act,layout,ids[pos])));
        close.setOnClickListener(v->MainActivity.doonback());present(act,parent,layout);
    }
    public static void details(MainActivity act,View parent,int group) {
        safely(act,()->{
            double[] root=ActivityNative.record(group);if(root==null)throw new IllegalStateException(act.getString(R.string.activity_unavailable));
            StringBuilder out=new StringBuilder();
            for(int source:ActivityNative.sources(group)) {
                double[] r=ActivityNative.record(source);String[] text=ActivityNative.texts(source);
                out.append(ActivityHealth.typeName((int)r[2])).append("\n").append(date((long)r[3])).append(" – ").append(date((long)r[4])).append('\n');
                if(!text[2].isEmpty())out.append(text[2]).append('\n');
                out.append(act.getString(R.string.activity_source)).append(": ").append(text[1]).append('\n');
                if(!text[4].isEmpty())out.append(text[4]).append('\n');
                if((((int)r[1])&1)!=0)out.append(act.getString(R.string.activity_source_deleted)).append('\n');
                else if((((int)r[1])&256)!=0)out.append(act.getString(R.string.activity_not_returned)).append('\n');
                String[] fields=act.getResources().getStringArray(R.array.activity_fields);
                for(int i=0;i<12;++i)if(Double.isFinite(r[6+i]))out.append(fields[i]).append(": ").append(String.format(Locale.getDefault(),"%.1f",r[6+i])).append('\n');
                if(Double.isFinite(r[9])&&Double.isFinite(r[8]))out.append(act.getString(R.string.activity_excess)).append(": ").append(String.format(Locale.getDefault(),"%.1f bpm",r[9]-r[8])).append('\n');
                double[] heart=ActivityNative.heartSources(source);
                for(int i=0;i<heart.length;i+=4) {
                    out.append('\n').append(act.getString(R.string.activity_heart_source)).append(": ").append(ActivityNative.sourceName((int)heart[i])).append('\n');
                    if(heart.length>4) {
                        out.append(fields[3]).append(": ").append(String.format(Locale.getDefault(),"%.1f",heart[i+1])).append('\n');
                        out.append(fields[4]).append(": ").append(String.format(Locale.getDefault(),"%.1f",heart[i+2])).append('\n');
                        out.append(fields[5]).append(": ").append(String.format(Locale.getDefault(),"%.1f",heart[i+3])).append('\n');
                    }
                }
                if(!text[3].isEmpty())out.append(text[3]).append('\n');
                out.append('\n');
            }
            TextView body=getlabel(act,out.toString());body.setTextIsSelectable(true);
            ScrollView scroll=new ScrollView(act);scroll.addView(body);
            Button mapping=getbutton(act,R.string.activity_labels),link=getbutton(act,R.string.activity_link),close=getbutton(act,R.string.closename);
            Layout layout=contentLayout(act,scroll,new View[]{mapping,link,close});
            mapping.setOnClickListener(v->mapping(act,layout,(int)root[2],null));
            link.setVisibility((((int)root[1])&2)!=0?View.GONE:View.VISIBLE);
            link.setOnClickListener(v->safely(act,()->chooseAmount(act,group)));
            close.setOnClickListener(v->MainActivity.doonback());present(act,parent,layout);
        });
    }
    private static void chooseAmount(MainActivity act,int group) {
        String[] candidates=ActivityNative.candidates(group);String[] display=new String[candidates.length];
        ArrayList<String> labels=Natives.getLabels();
        for(int i=0;i<candidates.length;++i){String[] parts=candidates[i].split(":");display[i]=date(Long.parseLong(parts[2]))+"  "+labels.get(Integer.parseInt(parts[3]))+"  "+parts[4];}
        if(display.length==0){Toast.makeText(act,R.string.activity_no_match,Toast.LENGTH_LONG).show();return;}
        new AlertDialog.Builder(act).setTitle(R.string.activity_link).setItems(display,(dialog,pos)->safely(act,()->{
            String[] p=candidates[pos].split(":");ActivityNative.link(group,Integer.parseInt(p[0]),Integer.parseInt(p[1]),Long.parseLong(p[2]),Integer.parseInt(p[3]),Float.parseFloat(p[4]));MainActivity.doonback();
        })).setNegativeButton(R.string.cancel,null).show();
    }
}
