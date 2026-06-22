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
/*      Sun Mar 10 11:40:55 CET 2024                                                 */


package tk.glucodata;

import static android.media.AudioAttributes.USAGE_ASSISTANCE_SONIFICATION;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.DontTalk;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.Natives.getInvertColors;
import static tk.glucodata.Natives.getVoicePitch;
import static tk.glucodata.Natives.getVoiceSeparation;
import static tk.glucodata.Natives.getVoiceSpeed;
import static tk.glucodata.Natives.getVoiceTalker;
import static tk.glucodata.Natives.lastglucose;
import static tk.glucodata.Natives.settouchtalk;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.getGenSpin;
import static tk.glucodata.settings.Settings.getProfileSpinner;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.settings.Settings.scheduleProfiles;
import static tk.glucodata.settings.Settings.setProfile;
import static tk.glucodata.settings.Settings.str2float;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.getlocale;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.os.Build;
import android.speech.tts.TextToSpeech;
import android.speech.tts.UtteranceProgressListener;
import android.speech.tts.Voice;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Locale;
import java.util.Set;

public class Talker {
static public final String LOG_ID="Talker";
    private TextToSpeech engine;

static    private float curpitch=1.0f;
static  private float curspeed=1.0f;
static private    long   cursep=50*1000L;
static private int voicepos=-1;
static private String playstring=null;
static private Spinner spinner=null;
//static final private int minandroid=24; //21
static final private int minandroid=21; //21

static void getvalues() {
if(!DontTalk) {
    float speed=getVoiceSpeed( );
    if(speed!=0.0f) {
        voicepos=getVoiceTalker( );
        cursep=getVoiceSeparation( )*1000L;
        curspeed=speed;
        curpitch=getVoicePitch( );
        SuperGattCallback.dotalk= Natives.getVoiceActive();
        }
        }
    }

static final private ArrayList<Voice> voiceChoice=new ArrayList<>();
void setvalues() {
if(!DontTalk) {
   var gine=engine;
    if(gine!=null) {
        var loc=getlocale();
        gine.setLanguage(loc);
        gine.setPitch( curpitch);
        gine.setSpeechRate( curspeed);
        }
       }
    }
void setvoice() {
if(!DontTalk) {
    if(engine==null)
        return;
    if(voicepos>=0&& voicepos<voiceChoice.size()) {
        var vo= voiceChoice.get(voicepos);
        engine.setVoice(voiceChoice.get(voicepos));
        {if(doLog) {Log.i(LOG_ID,"after setVoice "+vo.getName());};};
        }
    else {
        {if(doLog) {Log.i(LOG_ID,"setVoice out of range");};};
        voicepos=0;
        }
      }
    }
void destruct() {
if(!DontTalk) {
    if(engine!=null) {
        engine.shutdown();
        engine=null;
        }
    voiceChoice.clear();
    }
    }

 Talker(Context cont) {
if(!DontTalk) {

//    var context=cont==null?Applic.getContext():cont;
    var context=Applic.app;
     engine=new TextToSpeech(context, new TextToSpeech.OnInitListener() {
       @Override
      public void onInit(int status) {
        
       try {
         var gine=engine;
         if(gine==null) {
            Log.e(LOG_ID,"engine==null");
            return;
            }
         if(status ==TextToSpeech.SUCCESS) {
            setvalues();
            if (android.os.Build.VERSION.SDK_INT >= minandroid) {
                Set<Voice> voices=gine.getVoices();
                if(voices==null) {
                    {if(doLog) {Log.i(LOG_ID,"voices==null");};};
                    }
                else {
                    var loc=getlocale();
   //                    var lang=(context!=null)?context.getString(R.string.language):loc.getLanguage();
                    var lang=loc.getLanguage();
                    {if(doLog) {Log.i(LOG_ID,"lang="+lang);};};

                    voiceChoice.clear();
                    for(var voice:voices) {
                        if(lang.equals(voice.getLocale().getLanguage())) {
                            voiceChoice.add(voice);
                            }
                        }
                    var spin=spinner;
                    if(spin!=null) {
                        {if(doLog) {Log.i(LOG_ID,"Talker spinner!=null");};};
                          Applic.RunOnUiThread(() -> {
                            spin.setAdapter(new RangeAdapter<Voice>(voiceChoice, Applic.app, voice -> {
                                    return voice.getName();
                                    })); 
                            if(voicepos>=0&&voicepos<voiceChoice.size())
                                spin.setSelection(voicepos);
                            });
                        }
                    }
                setvoice();
            }
             if(playstring!=null) {
                 speak(playstring);
                 playstring=null;
                 }
           }
         else {
             Log.e(LOG_ID,"status = TextToSpeech.ERROR ");
             }
         {if(doLog) {Log.i(LOG_ID,"after onInit");};};
          }
       catch(Throwable th) {
         Log.stack(LOG_ID,"Talker.onInit",th);
         }
            }
          });
    engine.setOnUtteranceProgressListener(new UtteranceProgressListener() {

 //  AudioFocusRequest audiofocusrequest = (android.os.Build.VERSION.SDK_INT <26)?null:new AudioFocusRequest.Builder( AudioManager.AUDIOFOCUS_GAIN_TRANSIENT).setAudioAttributes( notification_audio ).build();
        @Override
        public void onDone(String utteranceId) {
            if(doLog) {Log.i(LOG_ID,"onDone "+utteranceId);};
            if(!notifyfocus)
                doTurnFocusoff();
        }

        @Override
        public void onError(String utteranceId) {
            if(doLog) {Log.i(LOG_ID,"onError "+utteranceId);};
            if(!notifyfocus)
                doTurnFocusoff();

            }
        @Override
        public void onStart(String utteranceId) {
            if(doLog) {Log.i(LOG_ID,"onStart "+utteranceId);};
            if(!notifyfocus)
                doTurnFocuson() ;
            }

    });
    if(doLog) {Log.i(LOG_ID,"after new TextToSpeech");};
    if(android.os.Build.VERSION.SDK_INT >= minandroid)
        engine.setAudioAttributes(talk_audio);
        }
   }

public void speak(String message) {
    if(!DontTalk) {
        try {
            if(
                    ((android.os.Build.VERSION.SDK_INT >= 21)?
                    engine.speak(message, TextToSpeech.QUEUE_FLUSH, null,message):
                            engine.speak(message, TextToSpeech.QUEUE_FLUSH, null)) ==TextToSpeech.SUCCESS)


            {
                if(doLog) {Log.i(LOG_ID,"success speak "+message);}
                }
             else {
                Log.e(LOG_ID,"failed speak "+message);
                }
            }
        catch(Throwable th) {
            Log.stack(LOG_ID,"speak failed",th);
            }
        }
    }
static boolean notifyfocus=false;
//private static final AudioAttributes notification_audio = (new AudioAttributes.Builder()) .setLegacyStreamType(TextToSpeech.Engine.DEFAULT_STREAM) .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH) .build(); 
//private static final AudioAttributes notification_audio =(android.os.Build.VERSION.SDK_INT >= 21)?new AudioAttributes.Builder().setUsage(isWearable? USAGE_ASSISTANCE_SONIFICATION:USAGE_NOTIFICATION ) .build():null;
//private static final AudioAttributes notification_audio = notification_audio;

static AudioAttributes talk_audio;
public void speak(String message, AudioAttributes attr) {
if(!DontTalk) {
    if(android.os.Build.VERSION.SDK_INT >= minandroid) {
        if(attr!=talk_audio) {
            engine.setAudioAttributes(attr);
           }
           }
    
//    engine.speak(message, TextToSpeech.QUEUE_FLUSH, null);
    speak(message);
    if(android.os.Build.VERSION.SDK_INT >= minandroid) {
        if(attr!=talk_audio)
            engine.setAudioAttributes(talk_audio);
         }
         }
    }
static long nexttime=0L;
void selspeak(String message) {
    if(!DontTalk) {
        var now=System.currentTimeMillis();    
        if(now>nexttime) {
            nexttime=now+cursep;
            speak(message);
            }
          }
    }
static final private double base2=Math.log(2);
static final private double multiplyer=10000.0/base2;
private static int ratio2progress(float ratio) {
    if(DontTalk)
        return 0;
     else {
        if(ratio<0.18)
            return 0;
        return (int)Math.round(Math.log(ratio)*multiplyer)+25000;
          }
    }
private static float progress2ratio(int progress) {
    if(DontTalk)
        return 0;
     else {
        return (float)Math.exp((double)(progress-25000)/multiplyer);
        }
    }

private static View[] slider(MainActivity context,float init) {
    if(!DontTalk) {
        var speed=new SeekBar(context);
     Applic.ifRTLseekbar(speed);
    //    speed.setMin(-25000);
        speed.setMax(50000);
        speed.setProgress(ratio2progress(init));
        var displayspeed=new EditText(context);
    //    displayspeed.setPadding(0,0,0,0);
            displayspeed.setImeOptions(editoptions);
        displayspeed.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        displayspeed.setMinEms(2);
        String formstr=String.format(Locale.US, "%.2f",init);
        speed.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
        displayspeed.setText( formstr);

            
        speed.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public  void onProgressChanged (SeekBar seekBar, int progress, boolean fromUser) {
                float num=progress2ratio(progress);
                String form=String.format(Locale.US, "%.2f",num);
                displayspeed.setText( form);
                {if(doLog) {Log.i(LOG_ID,"onProgressChanged "+form);};};
                }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                {if(doLog) {Log.i(LOG_ID,"onStartTrackingTouch");};};
                }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                {if(doLog) {Log.i(LOG_ID,"onStopTrackingTouch");};};
                }
            });

            displayspeed.setOnEditorActionListener( new TextView.OnEditorActionListener() {
                @Override
                public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER || actionId == EditorInfo.IME_ACTION_DONE) {
                     {if(doLog) {Log.i(LOG_ID,"onEditorAction");};};
                    var speedstr=v.getText().toString();
                    if(speedstr != null) {
                        float    curspeed = str2float(speedstr);
                        speed.setProgress(ratio2progress(curspeed));
                        {if(doLog) {Log.i(LOG_ID,"onEditorAction: "+speedstr+" "+curspeed);};};
                         tk.glucodata.help.hidekeyboard(context);
                        }
                    return true;
                   }
                return false;
                }});
                /*
            displayspeed.addTextChangedListener( new TextWatcher() {
                       public void afterTextChanged(Editable ed) {
                var speedstr=ed.toString();
                if(speedstr != null) {
                    float    curspeed = str2float(speedstr);
                    speed.setProgress(ratio2progress(curspeed));
                    {if(doLog) {Log.i(LOG_ID,"afterTextChanged: "+speedstr+" "+curspeed);};};
                    }

                           }
                       public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
                       public void onTextChanged(CharSequence s, int start, int before, int count) { }
                      });*/

        return new View[] {speed,displayspeed};
        }
    else return null;
    }
public static boolean shouldtalk() {
    if(!DontTalk) {
        return  SuperGattCallback.dotalk||Natives.gettouchtalk()||Natives.speakmessages()||Natives.speakalarms();
        }
    else
        return false;
    }
public  static boolean istalking() {
    if(DontTalk)
        return false;
    else
        return SuperGattCallback.talker!=null;
    }
public static void config(MainActivity context) {
    if(!DontTalk) {
        if(!istalking()) {
             SuperGattCallback.newtalker(context);
            }

        var separation=new EditText(context);

        separation.setImeOptions(editoptions);
        separation.setInputType(InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL);
        separation.setMinEms(2);
        int sep=(int)(cursep/1000L);
        separation.setText(sep+"");
        var seplabel=getlabel(context,context.getString(R.string.secondsbetween));
        float density=GlucoseCurve.metrics.density;
        int pad=(int)(density*3);
        seplabel.setPaddingRelative(pad*3,0,0,0);
        var speeds=slider(context,curspeed);

        var pitchs=slider(context,curpitch);
        var cancel=getbutton(context,R.string.cancel);

        var save=getbutton(context,R.string.save);
        var width= GlucoseCurve.getwidth();
        var speedlabel=getlabel(context,context.getString(R.string.speed));
        speedlabel.setPaddingRelative(pad,0,pad*2,0);
        var pitchlabel=getlabel(context,context.getString(R.string.pitch));
        pitchlabel.setPaddingRelative(pad,0,pad*2,0);
        var voicelabel=getlabel(context,context.getString(R.string.talker));
        var active=getcheckbox(context,R.string.speakglucose, SuperGattCallback.dotalk);
        active.setPaddingRelative(0,0,pad*3,0);

        var mediasound=getcheckbox(context,"MEDIA", Natives.getSoundType( )==AudioAttributes.USAGE_MEDIA);
        mediasound.setOnCheckedChangeListener(
                 (buttonView,  isChecked) -> {
                     if (isChecked) {
                         Natives.setSoundType(AudioAttributes.USAGE_MEDIA);
                     } else {
                         Natives.setSoundType(isWearable ? USAGE_ASSISTANCE_SONIFICATION : AudioAttributes.USAGE_NOTIFICATION);
                     }
                     maketalk_audio();
                      }
                     );
                        


        var test=getbutton(context,context.getString(R.string.test));
        if(spinner!=null) {
                {if(doLog) {Log.i(LOG_ID, "Talker.config spinner=!null");};};
                try {
                ViewGroup par = (ViewGroup) spinner.getParent();
                if (par != null)
                    par.removeView(spinner);
                }
            catch (Throwable th) {
                Log.stack(LOG_ID,"spinner",th);
                }
            }
        else
                Log.i(LOG_ID,"Talker.config spinner==null");  
        var spin= spinner!=null?spinner:((android.os.Build.VERSION.SDK_INT >= minandroid)? (spinner=getGenSpin(context)):null);

        int[] spinpos={-1};
        var touchtalk= getcheckbox(context,context.getString(R.string.talk_touch), Natives.gettouchtalk());
        var speakmessages= getcheckbox(context,context.getString(R.string.speakmessages), Natives.speakmessages());
        var speakalarms= getcheckbox(context,context.getString(R.string.speakalarms), Natives.speakalarms());
        var pro=getProfileSpinner(context);
       int curProfile=Natives.getProfile();
        pro.setSelection(curProfile);
       pro.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
        @Override
        public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
            if(position!=curProfile) {
               setProfile(context,position);
               MainActivity.doonback();
               config(context);
               }
            }
        @Override
        public  void onNothingSelected (AdapterView<?> parent) {

        } });
        if(android.os.Build.VERSION.SDK_INT >= minandroid) { 
            spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
                    {if(doLog) {Log.i(LOG_ID,"onItemSelected "+position);};};
                    
                    spinpos[0]=position;
                    }
                @Override
                public  void onNothingSelected (AdapterView<?> parent) {

                } });
            spin.setAdapter(new RangeAdapter<Voice>(voiceChoice, context, voice -> {
                    return voice.getName();
                    }));
            {if(doLog) {Log.i(LOG_ID,"voicepos="+voicepos);};};
            if(voicepos>=0&&voicepos<voiceChoice.size())
                spin.setSelection(voicepos);
            spinpos[0]=-1;
            }
        ViewGroup layout;
    var schedules=getbutton(context,R.string.schedules);

        if(isWearable) {
            int marg=(int)(width*.1f);
             Layout.getMargins(voicelabel).topMargin=marg;
             Layout.getMargins(cancel).setMarginStart(marg);
             Layout.getMargins(test).setMarginEnd(marg);
             Layout.getMargins(separation).setMarginEnd(marg);
            var lay=new Layout(context,(l,w,h)-> {
                return new int[] {w,h};
                },

                 new View[]{voicelabel},new View[]{spin},new View[]{active},new View[]{seplabel,separation}, new View[]{touchtalk},new View[]{ speakmessages},new View[]{speakalarms },new View[]{speedlabel},new View[]{speeds[1],speeds[0]},new View[]{pitchlabel},new View[]{pitchs[1],pitchs[0]},new View[]{mediasound},  new View[]{pro},new View[]{schedules},new View[]{cancel,test},new View[]{save});

            var scroll=new ScrollView(context);
            scroll.addView(lay);
            scroll.setFillViewport(true);
            scroll.setSmoothScrollingEnabled(false);
           scroll.setScrollbarFadingEnabled(true);
           scroll.setVerticalScrollBarEnabled(Applic.scrollbar);
            layout=scroll;
           }
        else {
            View[]  firstrow; 
            if(android.os.Build.VERSION.SDK_INT >= minandroid) { 
                 firstrow=new View[]{active,seplabel,separation,voicelabel,spin};
                 }
            else {
                var space=new Space(context);
                space.setMinimumWidth((int)(width*0.4));
                firstrow=new View[]{active,seplabel,separation,space};
              }
            var secondrow=new View[]{touchtalk, speakmessages, speakalarms };
            var marghor= (int)(width*.05f);

            Layout.getMargins(cancel).setMarginStart(marghor);
            Layout.getMargins(save).setMarginEnd(marghor);
            var helpview=getbutton(context,R.string.helpname);
            helpview.setOnClickListener(v-> help.help(R.string.talkhelp,context));
            layout= new Layout(context,(l,w,h)-> {
                return new int[] {w,h};
                },firstrow,secondrow,new View[]{speedlabel,speeds[1],speeds[0],pro},new View[]{pitchlabel,pitchs[1],pitchs[0],mediasound}, new View[]{cancel,helpview,schedules,test,save});
            }

        final var lay=layout;
        schedules.setOnClickListener(v->  {
            scheduleProfiles(context,lay);
        });
          //layout.setBackgroundResource(R.drawable.dialogbackground);
        layout.setBackgroundColor( Applic.backgroundcolor);
          context.themeLightBars();
        MainActivity.setonback(()-> {
            tk.glucodata.help.hidekeyboard(context);
            removeContentView(lay);
            spinner=null;
            if(Menus.on)
                Menus.show(context);

               context.lightBars(!getInvertColors( ));
            });
        cancel.setOnClickListener(v->  {
            context.doonback();
            });
        Runnable getvalues=()-> {
            try {
                if (android.os.Build.VERSION.SDK_INT >= minandroid) {
                    int pos=spinpos[0];
                    if(pos>=0) {
                        voicepos=pos;
                        }
                      }
                 var str = separation.getText().toString();
                if(str != null) {
                    cursep = Integer.parseInt(str)*1000L;
                    var now=System.currentTimeMillis();    
                    nexttime=now+cursep;
                    }
                var speedstr=((EditText)speeds[1]).getText().toString();
                if(speedstr != null) {
                    curspeed = str2float(speedstr);
                    {if(doLog) {Log.i(LOG_ID,"speedstr: "+speedstr+" "+curspeed);};};
                    }
                var pitchstr=((EditText)pitchs[1]).getText().toString();
                if(pitchstr != null) {
                    curpitch = str2float(pitchstr);
                    {if(doLog) {Log.i(LOG_ID,"pitchstr: "+pitchstr+" "+curpitch);};};
                    }
    ;
                } catch(Throwable th) {
                    Log.stack(LOG_ID,"parseInt",th);
                    }

             };
        save.setOnClickListener(v->  {
            getvalues.run();
            
            if(active.isChecked()||touchtalk.isChecked()||speakmessages.isChecked()||speakalarms.isChecked()) {
                SuperGattCallback.newtalker(context);
                SuperGattCallback.dotalk = active.isChecked();
                settouchtalk(touchtalk.isChecked());
                Natives.setspeakmessages(speakmessages.isChecked());
                Natives.setspeakalarms(speakalarms.isChecked());
                }
            else {
                settouchtalk(false);
                Natives.setspeakmessages(false);
                Natives.setspeakalarms(false);
                SuperGattCallback.endtalk();
                }
            Natives.saveVoice(curspeed,curpitch,(int)(cursep/1000L),voicepos,SuperGattCallback.dotalk);

            context.doonback();
            });
        test.setOnClickListener(v->  {
            var gl=lastglucose();
            var say=(gl!=null&&gl.value!=null)?gl.value:"8.7";
            getvalues.run();
            if(istalking()) {
                var talk=SuperGattCallback.talker;
                if(talk!=null) {
                    talk.setvalues();
                    talk.setvoice();
                    talk.speak(say);
                    return;
                    }
                }
            playstring=say;
            SuperGattCallback.newtalker(context);
            });


    //      layout.setPadding(MainActivity.systembarLeft,MainActivity.systembarTop/2,MainActivity.systembarRight,0);
    //    context.addMyContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT, WRAP_CONTENT));
        var top=MainActivity.systembarTop;
         
        var left=MainActivity.systembarStart;
          layout.setPaddingRelative(left+(int)(density*5.0),top,MainActivity.systembarEnd+(int)(density*8.0),MainActivity.systembarBottom);
    /*    var layheight=GlucoseCurve.getheight()-MainActivity.systembarBottom;
        var laywidth=GlucoseCurve.getwidth()-left-MainActivity.systembarRight;
        layout.setX(left); */
        context.addMyContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
        }
  }
static public void maketalk_audio() {
    if(android.os.Build.VERSION.SDK_INT >= 21) {
        int type=Natives.getSoundType( );
        Log.i(LOG_ID,"getSoundType()="+type);
        if(type==0) {
            type=isWearable? USAGE_ASSISTANCE_SONIFICATION: AudioAttributes.USAGE_NOTIFICATION;
            } 
        talk_audio=new AudioAttributes.Builder().setUsage(type).build();
        if(android.os.Build.VERSION.SDK_INT >= 26) {
                talkaudiofocusrequest = new AudioFocusRequest.Builder( AudioManager.AUDIOFOCUS_GAIN_TRANSIENT).setAudioAttributes( talk_audio ).build();
                Log.i(LOG_ID, "audiofocusrequest  has value");
                }
         else {
                talkaudiofocusrequest =null;
                Log.i(LOG_ID, "audiofocusrequest=null");
                }
        }
    else {
                talk_audio=null;
        }
    }

static private AudioFocusRequest talkaudiofocusrequest;
static private boolean turnfocusoff=false;
        static void doTurnFocusoff() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                final var wasturnoff=turnfocusoff;
                turnfocusoff=false;
                if(wasturnoff) {
                    Notify.audioManager.abandonAudioFocusRequest(talkaudiofocusrequest);
                }
            }
        }

        static void doTurnFocuson() {
            if(android.os.Build.VERSION.SDK_INT >= 26) {
                if(!turnfocusoff) {
                    switch(Notify.audioManager.requestAudioFocus(talkaudiofocusrequest)) {
                        case AudioManager.AUDIOFOCUS_REQUEST_FAILED:
                            Log.i(LOG_ID,"REQUEST_FAILED");
                            break;

                        case AudioManager.AUDIOFOCUS_REQUEST_GRANTED:
                            turnfocusoff=true;
                            Log.i(LOG_ID,"REQUEST_GRANTED");
                            break;
                        case AudioManager.AUDIOFOCUS_REQUEST_DELAYED:
                            Log.i(LOG_ID,"REQUEST_DELAYED");
                            break;
                    };
                }
            }
        }


static {
 maketalk_audio();
}
}
/*
TODO
van EditText naar Slider
Hoe met test?
Probleem: veranderingen moeten eerst uitgevoerd zijn voordat test mogelijk is.
Mogelijkheden:
- 
*/
