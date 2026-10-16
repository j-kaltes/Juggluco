/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

import androidx.appcompat.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Typeface;
import android.os.Handler;
import android.os.Looper;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.InputType;
import android.text.style.RelativeSizeSpan;
import android.text.style.StyleSpan;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
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
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;
import static tk.glucodata.ActivityFormat.quantity;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.help.help;


/** Activity dialogs use the same Layout and theme as the other Juggluco views. */
public final class ActivitySettings {
    private ActivitySettings() {}
    private static final String LOG_ID="ActivitySettings";
    private static final int[] intervals={0,900,1800,3600,10800,21600,86400};
    private static String date(long seconds) {
        return DateFormat.getDateTimeInstance(DateFormat.SHORT,DateFormat.SHORT).format(new Date(seconds*1000));
    }
    private static String message(Exception e) {
        return e.getMessage()==null?e.toString():e.getMessage();
    }
    private static Spinner spinner(MainActivity act,String[] values) {
        Spinner view=tk.glucodata.settings.Settings.getGenSpin(act);
        view.setAdapter(new LabelAdapter<String>(act,Arrays.asList(values),0));
        return view;
    }
    private static void presentrun(MainActivity act,View parent,ViewGroup layout,Runnable extra) {
        presentrun(act,parent,layout,extra,null);
    }
    private static void presentrun(MainActivity act,View parent,ViewGroup layout,Runnable extra,BooleanSupplier changed) {
        if(parent!=null)RingTones.EnableControls(parent,false);
        layout.setBackgroundColor(Applic.backgroundcolor);
        final int sidepad=GlucoseCurve.dpToPx(8);
        final int pad=GlucoseCurve.dpToPx(2);
        if(layout instanceof Layout)
            ((Layout)layout).systembarPadding((l,t,r,b)->new int[]{l+sidepad,t+pad,r+sidepad,b+pad});
        act.addMyContentView(layout,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
        Runnable close=()->{
            if(extra!=null)
                    extra.run();
            tk.glucodata.settings.Settings.removeContentView(layout);
            if(parent!=null)RingTones.EnableControls(parent,true);
            act.requestRender();
        };
        if(changed==null)MainActivity.setonback(close);
        else MainActivity.setonback(new ActivityEditGuard(changed,new ActivityEditGuard.BackStack(){
            public void push(Runnable callback){MainActivity.setonback(callback);}
            public void pop(){MainActivity.poponback();}
        },(discard,dismissed)->new AlertDialog.Builder(act).setTitle(R.string.withoutsaving)
            .setPositiveButton(R.string.ok,(dialog,which)->discard.run())
            .setNegativeButton(R.string.cancel,null).setOnDismissListener(dialog->dismissed.run()).show(),close));
    }
    private static void present(MainActivity act,View parent,ViewGroup layout) {
          presentrun(act,parent,layout,null);
        }
    public static void show(MainActivity act,View parent) {
        String operation="Open activity settings";
        Log.i(LOG_ID,operation);
        try {
            long[] config=ActivityNative.config();
            long[] from={config[0]==0?System.currentTimeMillis()/1000-7*86400:config[0]};
            TextView title=getlabel(act,R.string.activity_import);
            Button since=getbutton(act,date(from[0]));
            ActivityDatePicker datePicker=new ActivityDatePicker(act,(picker,y,m,d)->{
                Calendar cal=Calendar.getInstance();cal.set(y,m,d,0,0,0);cal.set(Calendar.MILLISECOND,0);
                from[0]=cal.getTimeInMillis()/1000;since.setText(date(from[0]));
            });
            since.setOnClickListener(v->datePicker.show(from[0]));
            Spinner interval=spinner(act,act.getResources().getStringArray(R.array.activity_intervals));
            for(int i=0;i<intervals.length;++i)if(intervals[i]==config[1])interval.setSelection(i);
            CheckDirectionBox amounts=util.getcheckbox(act,R.string.activity_create_amounts,(config[5]&ActivityNative.CREATE_AMOUNTS)!=0);
            Spinner units=spinner(act,act.getResources().getStringArray(R.array.activity_units));
            units.setSelection((config[5]&ActivityNative.USE_MILES)!=0?1:0);
            TextView status=getlabel(act,"");
            Button retrieve=getbutton(act,R.string.activity_retrieve),labels=getbutton(act,R.string.activity_labels);
            var help=getbutton(act,R.string.helpname);
            help.setOnClickListener(v-> tk.glucodata.help.help(R.string.activities,act));
            retrieve.setEnabled(!Natives.staticnum());
            Button list=getbutton(act,R.string.activity_saved),save=getbutton(act,R.string.save),close=getbutton(act,R.string.closename);
            Layout form=new Layout(act,
                new View[]{getlabel(act,R.string.activity_from),since,getlabel(act,R.string.activity_interval),interval},
                new View[]{amounts,labels},
                new View[]{list,getlabel(act,R.string.activity_units),units},
                new View[]{status},
                new View[]{save,retrieve,help,close}).
            portraitLayout(
                new View[]{getlabel(act,R.string.activity_from),since},
                new View[]{getlabel(act,R.string.activity_interval),interval},
                new View[]{amounts},
                new View[]{getlabel(act,R.string.activity_units),units},
                new View[]{retrieve},
                new View[]{labels,list},
                new View[]{status},
                new View[]{save,help,close});
            status.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));

            final int sidepad=GlucoseCurve.dpToPx(8);
            final int pad=GlucoseCurve.dpToPx(2);
            form.systembarPadding((l,t,r,b)->new int[]{l+sidepad,t+pad,r+sidepad,b+pad});
            ScrollView scroll=new ScrollView(act);
            scroll.setFillViewport(true);
            scroll.setSmoothScrollingEnabled(true);
            scroll.setVerticalScrollBarEnabled(Applic.scrollbar);
            scroll.setScrollbarFadingEnabled(true);
            scroll.addView(form,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
            ViewGroup layout=scroll;
            long[] savedSettings={from[0],intervals[interval.getSelectedItemPosition()],config[5]&(ActivityNative.CREATE_AMOUNTS|ActivityNative.USE_MILES)};
            Runnable apply=()->{
                long[] current=ActivityNative.config();
                int options=(amounts.isChecked()?ActivityNative.CREATE_AMOUNTS:0)|(units.getSelectedItemPosition()==1?ActivityNative.USE_MILES:0);
                Log.i(LOG_ID,"Activity options: createAmounts="+amounts.isChecked()+", units="+(units.getSelectedItemPosition()==1?"mi":"km"));
                ActivityNative.configure(from[0],intervals[interval.getSelectedItemPosition()],(int)current[3]);
                ActivityNative.setOptions(options);
                ActivityHealth.settingsChanged(act);
                ActivityHealth.numberEditorClosed();
                savedSettings[0]=from[0];savedSettings[1]=intervals[interval.getSelectedItemPosition()];savedSettings[2]=options;
            };
            save.setOnClickListener(v->{
                String operationSave="Save activity settings";
                Log.i(LOG_ID,operationSave);
                try {
                    apply.run();MainActivity.doonback();
                    String operationPermissions="Open activity permissions";
                    Log.i(LOG_ID,operationPermissions);
                    try {ActivityHealth.saved(act);} catch(Exception e) {
                        Log.stack(LOG_ID,operationPermissions,e);
                        Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                    }
                } catch(Exception e) {
                    Log.stack(LOG_ID,operationSave,e);
                    Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                }
            });
            retrieve.setOnClickListener(v->{
                String operationRetrieve="Retrieve activities";
                Log.i(LOG_ID,operationRetrieve);
                try {apply.run();ActivityHealth.retrieve(act);} catch(Exception e) {
                    Log.stack(LOG_ID,operationRetrieve,e);
                    Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                }
            });
           // help.setOnClickListener(v->tk.glucodata.help.help(act.getString(R.string.activity_amounts_consent)+"\n\n"+act.getString(R.string.activity_access),act));
            labels.setOnClickListener(v->{
                String operationLabels="Open activity label mappings";
                Log.i(LOG_ID,operationLabels);
                try {mappings(act,layout);} catch(Exception e) {
                    Log.stack(LOG_ID,operationLabels,e);
                    Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                }
            });
            list.setOnClickListener(v->{
                String operationList="Open saved activities";
                Log.i(LOG_ID,operationList);
                try {activities(act,layout);} catch(Exception e) {
                    Log.stack(LOG_ID,operationList,e);
                    Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                }
            });
            close.setOnClickListener(v->MainActivity.doonback());
            layout.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener(){
                public void onViewAttachedToWindow(View view){}
                public void onViewDetachedFromWindow(View view){datePicker.close();}
            });
            presentrun(act,parent,layout,datePicker::close,()->from[0]!=savedSettings[0]||
                intervals[interval.getSelectedItemPosition()]!=savedSettings[1]||
                ((amounts.isChecked()?ActivityNative.CREATE_AMOUNTS:0)|(units.getSelectedItemPosition()==1?ActivityNative.USE_MILES:0))!=savedSettings[2]);
            Handler handler=new Handler(Looper.getMainLooper());
            Runnable refresh=new Runnable(){public void run(){
                if(!layout.isAttachedToWindow())return;
                try {
                    long last=ActivityNative.config()[2];
                    status.setText(act.getString(R.string.activity_last)+" "+(last==0?act.getString(R.string.activity_never):date(last))+"\n"+ActivityHealth.status());
                } catch(Exception e) {
                    Log.stack(LOG_ID,"Refresh activity status",e);
                    status.setText(message(e));
                    return; // Report once, instead of repeating the failure every second.
                }
                handler.postDelayed(this,1000);
            }};handler.post(refresh);
        } catch(Exception e) {
            Log.stack(LOG_ID,operation,e);
            Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
        }
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
        return new Layout(act,new View[]{list},new View[]{close});
    }
   
    private static void mapping(MainActivity act,View parent,int type,Runnable refreshed) {
        String operation="Open activity mapping: type="+type;
        Log.i(LOG_ID,operation);
        try {
            int[] rule=type<0?new int[]{(int)ActivityNative.config()[3],0}:ActivityNative.rule(type);
            ArrayList<String> names=new ArrayList<>();ArrayList<Integer> ids=new ArrayList<>();
            ArrayList<String> all=Natives.getLabels();
            for(int i=0;i<all.size();++i)if(ActivityNative.labelAllowed(i)){names.add(all.get(i));ids.add(i);}
            Spinner label=spinner(act,names.toArray(new String[0]));
            int selected=ids.indexOf(rule[0]);if(selected>=0)label.setSelection(selected);
            Spinner measure=spinner(act,act.getResources().getStringArray(R.array.activity_measures));measure.setSelection(rule[1]);
            float initialRoundTo=type<0?1.0f:ActivityNative.roundTo(type);
            EditText roundTo=new EditText(act);roundTo.setSingleLine(true);roundTo.setMinEms(4);
            roundTo.setInputType(InputType.TYPE_CLASS_NUMBER|InputType.TYPE_NUMBER_FLAG_DECIMAL);
            roundTo.setImeOptions(tk.glucodata.settings.Settings.editoptions);
            roundTo.setText(Float.toString(initialRoundTo));
            final int initialLabel=label.getSelectedItemPosition();
            boolean[] saved={false};
            SharedPreferences preferences=act.getSharedPreferences("activity-mapping",Context.MODE_PRIVATE);
            String existingKey="update_existing_"+type;
            boolean initialExisting=preferences.getBoolean(existingKey,true);
            CheckDirectionBox existing=util.getcheckbox(act,R.string.activity_update_existing,initialExisting);
            Button add=getbutton(act,R.string.activity_new_label),
                save=getbutton(act,R.string.save),
                close=getbutton(act,R.string.closename),
                help=getbutton(act,R.string.helpname);

            help.setOnClickListener(v-> tk.glucodata.help.help(R.string.assignActivity,act));
            Layout form=new Layout(act,
                new View[]{getlabel(act,type<0?act.getString(R.string.activity_default):ActivityHealth.typeName(type)),label,add},
                type<0?null:new View[]{measure,getlabel(act,R.string.activity_round_to),roundTo},
                type<0?null:new View[]{existing},
                new View[]{save,help,close}
                ).portraitLayout(
                    new View[]{getlabel(act,type<0?act.getString(R.string.activity_default):ActivityHealth.typeName(type))},
                    new View[]{label,add},
                    type<0?null:new View[]{measure},
                    type<0?null:new View[]{getlabel(act,R.string.activity_round_to),roundTo},
                    type<0?null:new View[]{existing},
                    new View[]{save,help,close}
                    );
            final int sidepad=GlucoseCurve.dpToPx(8);
            final int pad=GlucoseCurve.dpToPx(2);
            form.systembarPadding((l,t,r,b)->new int[]{l+sidepad,t+pad,r+sidepad,b+pad});
            ScrollView scroll=new ScrollView(act);
            scroll.setFillViewport(true);
            scroll.setSmoothScrollingEnabled(true);
            scroll.setVerticalScrollBarEnabled(Applic.scrollbar);
            scroll.setScrollbarFadingEnabled(true);
            scroll.addView(form,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));


            ViewGroup layout=scroll;

            add.setOnClickListener(v->{EditText text=new EditText(act);
                text.setSingleLine(true);
                text.setImeOptions(tk.glucodata.settings.Settings.editoptions);
                AlertDialog dialog=new AlertDialog.Builder(act).setTitle(R.string.activity_new_label).setView(text)
                    .setPositiveButton(R.string.save,(d,which)->{
                        String operationCreate="Create activity label: type="+type;
                        Log.i(LOG_ID,operationCreate);
                        try {
                            int id=ActivityNative.newLabel(text.getText().toString().trim());
                            names.add(Natives.getLabels().get(id));ids.add(id);label.setAdapter(new LabelAdapter<String>(act,names,0));label.setSelection(ids.size()-1);
                        } catch(Exception e) {
                            Log.stack(LOG_ID,operationCreate,e);
                            Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                        }
                    }).setNegativeButton(R.string.cancel,null).show();
                text.setOnEditorActionListener((view,action,event)->{
                    if(action!=EditorInfo.IME_ACTION_DONE)return false;
                    dialog.getButton(AlertDialog.BUTTON_POSITIVE).performClick();
                    return true;
                });
            });
            save.setOnClickListener(v->{
                String operationSave="Save activity mapping: type="+type;
                Log.i(LOG_ID,operationSave);
                try {
                    if(ids.isEmpty())throw new IllegalStateException(act.getString(R.string.activity_choose_label));
                    int id=ids.get(label.getSelectedItemPosition());
                    float increment=initialRoundTo;
                    if(type>=0)try{increment=ActivityFormat.roundTo(roundTo.getText().toString());}
                        catch(IllegalArgumentException e){roundTo.setError(act.getString(R.string.activity_round_to_invalid));roundTo.requestFocus();return;}
                    Log.i(LOG_ID,"Activity mapping: type="+type+", label="+id+", measure="+measure.getSelectedItemPosition()+", roundTo="+increment+", existing="+existing.isChecked());
                    if(type<0) {
                           long[] cfg=ActivityNative.config();
                           ActivityNative.configure(cfg[0],(int)cfg[1],id);
                           }
                    else {
                        ActivityNative.setRule(type,id,measure.getSelectedItemPosition(),increment,existing.isChecked());
                        preferences.edit().putBoolean(existingKey,existing.isChecked()).apply();
                    }
                    saved[0]=true;
                    if(refreshed!=null)refreshed.run();
                    MainActivity.doonback();
                } catch(Exception e) {
                    Log.stack(LOG_ID,operationSave,e);
                    Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                }
            });
            close.setOnClickListener(v->MainActivity.doonback());
            presentrun(act,parent,layout,null,()->!saved[0]&&(label.getSelectedItemPosition()!=initialLabel||type>=0&&
                (measure.getSelectedItemPosition()!=rule[1]||existing.isChecked()!=initialExisting||ActivityFormat.roundingChanged(roundTo.getText().toString(),initialRoundTo))));
        } catch(Exception e) {
            Log.stack(LOG_ID,operation,e);
            Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
        }
    }
    private static void activities(MainActivity act,View parent) {
        ArrayList<Integer> ids=new ArrayList<>();ArrayList<String> rows=new ArrayList<>();
        ArrayAdapter<String> adapter=new ArrayAdapter<>(act,android.R.layout.simple_list_item_1,rows);
        Runnable refresh=()->{
            ids.clear();rows.clear();
            for(int id:ActivityNative.indices(false)){int source=ActivityNative.representative(id);double[] r=ActivityNative.record(source);String[] t=ActivityNative.texts(source);
                if(r==null||t==null)continue;
                ids.add(id);
                rows.add(date((long)r[3])+" · "+ActivityHealth.typeName((int)r[2])+(t[2].isEmpty()?"":" · "+t[2]));}
            adapter.notifyDataSetChanged();
        };refresh.run();
        if(ids.isEmpty()){Applic.argToaster(act,R.string.activity_none,Toast.LENGTH_LONG);return;}
        ListView list=new ListView(act);list.setAdapter(adapter);
        Button close=getbutton(act,R.string.closename);Layout layout=listLayout(act,list,close);
        list.setOnItemClickListener((a,v,pos,id)->details(act,layout,ids.get(pos),null,refresh));
        close.setOnClickListener(v->MainActivity.doonback());present(act,parent,layout);
    }
    public static void details(MainActivity act,View parent,int group) {
        details(act,parent,group,null,null);
    }
    private static void heading(SpannableStringBuilder out,String text,boolean activity) {
        int start=out.length();out.append(text);
        out.setSpan(new StyleSpan(Typeface.BOLD),start,out.length(),Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        if(activity)out.setSpan(new RelativeSizeSpan(1.35f),start,out.length(),Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        out.append('\n');
    }
    private static void field(SpannableStringBuilder out,String[] fields,int index,double value,boolean miles,Locale locale) {
        if(Double.isFinite(value))out.append(fields[index]).append(": ").append(quantity(index,value,miles,locale)).append('\n');
    }
    private static void peakField(SpannableStringBuilder out,String[] fields,int index,double value,double time,boolean miles,Locale locale) {
        if(Double.isFinite(value))out.append(fields[index]).append(": ").append(ActivityFormat.peakQuantity(index,value,(long)time,miles,locale)).append('\n');
    }
    private static void heartFields(SpannableStringBuilder out,String[] fields,String excess,
            double resting,double mean,double sd,double coverage,double maximum,double maximumTime,Locale locale) {
        field(out,fields,2,resting,false,locale);
        field(out,fields,3,mean,false,locale);
        peakField(out,fields,12,maximum,maximumTime,false,locale);
        if(Double.isFinite(mean)&&Double.isFinite(resting))
            out.append(excess).append(": ").append(quantity(3,mean-resting,false,locale)).append('\n');
        field(out,fields,4,sd,false,locale);
        if(Double.isFinite(mean))field(out,fields,5,coverage,false,locale);
    }
    static void details(MainActivity act,View parent,int group,Runnable amountsChanged,Runnable removed) {
        if(amountsChanged!=null)
            act.themeLightBars();
        String operation="Open activity details: group="+group;
        Log.i(LOG_ID,operation);
        try {
            double[] root=ActivityNative.record(group);if(root==null)throw new IllegalStateException(act.getString(R.string.activity_unavailable));
            double[] summary=ActivityNative.record(ActivityNative.representative(group));
            if(summary==null)throw new IllegalStateException(act.getString(R.string.activity_unavailable));
            SpannableStringBuilder out=new SpannableStringBuilder();
            Locale locale=Locale.getDefault();
            boolean miles=(ActivityNative.config()[5]&ActivityNative.USE_MILES)!=0;
            String[] fields=act.getResources().getStringArray(R.array.activity_fields);
            String excess=act.getString(R.string.activity_excess);
            for(int source:ActivityNative.sources(group)) {
                double[] r=ActivityNative.record(source);String[] text=ActivityNative.texts(source);
                heading(out,ActivityHealth.typeName((int)r[2]),true);
                if(!text[2].isEmpty())out.append(text[2]).append('\n');
                heading(out,text[1]+":",false);
                if((((int)r[1])&256)!=0)out.append(act.getString(R.string.activity_not_returned)).append('\n');
                out.append(act.getString(R.string.activity_period_duration,date((long)r[3]),date((long)r[4]),
                    ActivityFormat.duration(r[6],locale))).append('\n');
                // Keep movement quantities together, before the HR statistics.
                double[] peak=ActivityNative.peaks(source);
                for(int i:new int[]{1,7})field(out,fields,i,r[6+i],miles,locale);
                peakField(out,fields,13,peak[0],peak[1],miles,locale);
                for(int i:new int[]{8,6,9,10,11})field(out,fields,i,r[6+i],miles,locale);
                double[] heart=ActivityNative.heartSources(source);
                final int stride=ActivityNative.HEART_SOURCE_FIELDS;
                String[] names=new String[heart.length/stride];
                for(int i=0;i<names.length;++i)names[i]=ActivityNative.sourceName((int)heart[i*stride]);
                for(int i=0;i<heart.length;i+=stride) {
                    String name=names[i/stride];
                    if(i!=0||!ActivityFormat.sameSource(text[1],text[4],name)) {
                        // Show device names only to distinguish watches from
                        // the same app. Phone names and numeric type tags stay out.
                        String origin=ActivityFormat.sourceOrigin(name);
                        boolean distinguish=origin.equals(text[1]);
                        for(int n=0;n<names.length&&!distinguish;++n)
                            distinguish=n!=i/stride&&origin.equals(ActivityFormat.sourceOrigin(names[n]));
                        heading(out,ActivityFormat.sourceLabel(name,distinguish)+":",false);
                    }
                    heartFields(out,fields,excess,heart[i+4],heart[i+1],heart[i+2],heart[i+3],heart[i+5],heart[i+6],locale);
                }
                // Older cached records may have statistics without raw samples.
                if(heart.length==0)heartFields(out,fields,excess,r[8],r[9],r[10],r[11],r[20],peak[2],locale);
                if(!text[3].isEmpty())out.append(text[3]).append('\n');
                out.append('\n');
            }
            TextView body=getlabel(act,"");body.setText(out);body.setTextIsSelectable(true);
            body.setLineSpacing(GlucoseCurve.dpToPx(3),1.0f);
            ScrollView scroll=new ScrollView(act);
            scroll.addView(body);
            scroll.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            Button mapping=getbutton(act,R.string.activity_labels),link=getbutton(act,R.string.activity_link),sourceDelete=getbutton(act,R.string.activity_remove_source),delete=getbutton(act,R.string.delete),close=getbutton(act,R.string.closename);
            Layout layout=new Layout(act,new View[]{scroll},new View[]{mapping,link,sourceDelete,delete,close})
                .portraitLayout(new View[]{scroll},new View[]{link,sourceDelete},new View[]{mapping,delete,close});
            mapping.setOnClickListener(v->mapping(act,layout,(int)summary[2],amountsChanged));
            link.setVisibility((((int)root[1])&2)!=0?View.GONE:View.VISIBLE);
            link.setOnClickListener(v->{
                String operationLink="Choose activity Amount: group="+group;
                Log.i(LOG_ID,operationLink);
                try {chooseAmount(act,group);} catch(Exception e) {
                    Log.stack(LOG_ID,operationLink,e);
                    Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                }
            });
            sourceDelete.setOnClickListener(v->{
                String operationSource="Choose activity source: group="+group;
                Log.i(LOG_ID,operationSource);
                try {
                    int[] ids=ActivityNative.sourceIDs(group);String[] names=new String[ids.length];
                    for(int i=0;i<ids.length;++i)names[i]=ActivityFormat.sourceLabel(ActivityNative.sourceName(ids[i]),true);
                    new AlertDialog.Builder(act).setTitle(R.string.activity_remove_source).setItems(names,(dialog,index)->
                        new AlertDialog.Builder(act).setTitle(R.string.activity_remove_source)
                            .setMessage(act.getString(R.string.activity_remove_source_confirm,names[index]))
                            .setPositiveButton(R.string.delete,(confirmation,which)->{
                                String operationRemove="Remove activity source: group="+group;
                                Log.i(LOG_ID,operationRemove);
                                try {
                                    ActivityNative.removeSource(group,ids[index]);MainActivity.doonback();
                                    ActivityHealth.numberEditorClosed();act.requestRender();
                                    if(ActivityNative.representative(group)!=0)details(act,parent,group,amountsChanged,removed);
                                    else if(removed!=null)removed.run();
                                } catch(Exception e) {
                                    Log.stack(LOG_ID,operationRemove,e);
                                    Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                                }
                            }).setNegativeButton(R.string.cancel,null).show()
                    ).setNegativeButton(R.string.cancel,null).show();
                } catch(Exception e) {
                    Log.stack(LOG_ID,operationSource,e);
                    Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                }
            });
            delete.setOnClickListener(v->new AlertDialog.Builder(act).setTitle(R.string.deletequestion)
                .setMessage(R.string.activity_delete_confirm)
                .setPositiveButton(R.string.delete,(dialog,which)->{
                    String operationDelete="Delete activity: group="+group;
                    Log.i(LOG_ID,operationDelete);
                    try {
                        ActivityNative.remove(group);
                        MainActivity.doonback();
                        if(removed!=null)removed.run();
                        ActivityHealth.numberEditorClosed();
                        act.requestRender();
                    } catch(Exception e) {
                        Log.stack(LOG_ID,operationDelete,e);
                        Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
                    }
                }).setNegativeButton(R.string.cancel,null).show());
            close.setOnClickListener(v->MainActivity.doonback());
            presentrun(act,parent,layout,()-> {
                        if(amountsChanged!=null)
                                        act.lightBars(!Natives.getInvertColors( ));
                        });

        } catch(Exception e) {
            Log.stack(LOG_ID,operation,e);
            Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
        }
    }
    private static void chooseAmount(MainActivity act,int group) {
        String[] candidates=ActivityNative.candidates(group);String[] display=new String[candidates.length];
        ArrayList<String> labels=Natives.getLabels();
        for(int i=0;i<candidates.length;++i){String[] parts=candidates[i].split(":");display[i]=date(Long.parseLong(parts[2]))+"  "+labels.get(Integer.parseInt(parts[3]))+"  "+parts[4];}
        if(display.length==0){Applic.argToaster(act,R.string.activity_no_match,Toast.LENGTH_LONG);return;}
        new AlertDialog.Builder(act).setTitle(R.string.activity_link).setItems(display,(dialog,pos)->{
            String operation="Link activity Amount: group="+group;
            Log.i(LOG_ID,operation);
            try {
                String[] p=candidates[pos].split(":");ActivityNative.link(group,Integer.parseInt(p[0]),Integer.parseInt(p[1]),Long.parseLong(p[2]),Integer.parseInt(p[3]),Float.parseFloat(p[4]));MainActivity.doonback();
            } catch(Exception e) {
                Log.stack(LOG_ID,operation,e);
                Applic.argToaster(act,message(e),Toast.LENGTH_LONG);
            }
        }).setNegativeButton(R.string.cancel,null).show();
    }
}
