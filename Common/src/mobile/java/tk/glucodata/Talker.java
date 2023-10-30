package tk.glucodata;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Natives.getVoicePitch;
import static tk.glucodata.Natives.getVoiceSeparation;
import static tk.glucodata.Natives.getVoiceSpeed;
import static tk.glucodata.Natives.getVoiceTalker;
import static tk.glucodata.Natives.lastglucose;
import static tk.glucodata.Natives.settouchtalk;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.settings.Settings.str2float;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.getlocale;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.speech.tts.TextToSpeech;
import android.speech.tts.Voice;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.EditText;
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
static private	long   cursep=50*1000L;
static private int voicepos=-1;
static private String playstring=null;
static private Spinner spinner=null;
//static final private int minandroid=24; //21
static final private int minandroid=21; //21

static void getvalues() {
	float speed=getVoiceSpeed( );
	if(speed!=0.0f) {
		voicepos=getVoiceTalker( );
		cursep=getVoiceSeparation( )*1000L;
		curspeed=speed;
		curpitch=getVoicePitch( );
		SuperGattCallback.dotalk= Natives.getVoiceActive();
		}
	}

static private ArrayList<Voice> voiceChoice=new ArrayList();
void setvalues() {
	if(engine!=null) {
		var loc=getlocale();
		engine.setLanguage(loc);
	       engine.setPitch( curpitch);
	      engine.setSpeechRate( curspeed);
	      }
	}
void setvoice() {
	if(engine==null)
		return;
	if(voicepos>=0&& voicepos<voiceChoice.size()) {
		var vo= voiceChoice.get(voicepos);
		engine.setVoice(voiceChoice.get(voicepos));
		Log.i(LOG_ID,"after setVoice "+vo.getName());
		}
	else {
	    Log.i(LOG_ID,"setVoice out of range");
		voicepos=0;
		}
	}
void destruct() {
	if(engine!=null) {
		engine.shutdown();
		engine=null;
		}
	voiceChoice.clear();
	}
	Talker(Context context) {
	    engine=new TextToSpeech(Applic.app, new TextToSpeech.OnInitListener() {
		 @Override
		 public void onInit(int status) {
		    if(status != TextToSpeech.ERROR) {
		       setvalues();
			if (android.os.Build.VERSION.SDK_INT >= minandroid) {
				Set<Voice> voices=engine.getVoices();
				if(voices!=null) {
					var loc=getlocale();
//					var lang=(context!=null)?context.getString(R.string.language):loc.getLanguage();
					var lang=loc.getLanguage();
					Log.i(LOG_ID,"lang="+lang);

					voiceChoice.clear();
					for(var voice:voices) {
						if(lang.equals(voice.getLocale().getLanguage())) {
							voiceChoice.add(voice);
							}
						}
					var spin=spinner;
					if(spin!=null) {
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
		 	Log.e(LOG_ID,"getVoices()==null");
		 	}
		Log.i(LOG_ID,"after onInit");
		    }
	      });
}

public void speak(String message) {
	engine.speak(message, TextToSpeech.QUEUE_FLUSH, null);
	}
static long nexttime=0L;
void selspeak(String message) {
	var now=System.currentTimeMillis();	
	if(now>nexttime) {
		speak(message);
		nexttime=now+cursep;
		}
	}
static private double base2=Math.log(2);
static private double multiplyer=10000.0/base2;
static int ratio2progress(float ratio) {
	if(ratio<0.18)
		return 0;
        return (int)Math.round(Math.log(ratio)*multiplyer)+25000;
        }
static float progress2ratio(int progress) {
        return (float)Math.exp((double)(progress-25000)/multiplyer);
        }

private static View[] slider(MainActivity context,float init) {
	var speed=new SeekBar(context);
//	speed.setMin(-25000);
	speed.setMax(50000);
	speed.setProgress(ratio2progress(init));
	var displayspeed=new EditText(context);
//	displayspeed.setPadding(0,0,0,0);
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
			Log.i(LOG_ID,"onProgressChanged "+form);
			}
		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {
			Log.i(LOG_ID,"onStartTrackingTouch");
			}
		@Override
		public void onStopTrackingTouch(SeekBar seekBar) {
			Log.i(LOG_ID,"onStopTrackingTouch");
			}
		});

		displayspeed.setOnEditorActionListener( new TextView.OnEditorActionListener() {
		    @Override
		    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
		    if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER || actionId == EditorInfo.IME_ACTION_DONE) {
				 Log.i(LOG_ID,"onEditorAction");
				var speedstr=v.getText().toString();
				if(speedstr != null) {
					float	curspeed = str2float(speedstr);
					speed.setProgress(ratio2progress(curspeed));
					Log.i(LOG_ID,"onEditorAction: "+speedstr+" "+curspeed);
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
				float	curspeed = str2float(speedstr);
				speed.setProgress(ratio2progress(curspeed));
				Log.i(LOG_ID,"afterTextChanged: "+speedstr+" "+curspeed);
				}

                   	}
                   public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
                   public void onTextChanged(CharSequence s, int start, int before, int count) { }
                  });*/

	return new View[] {speed,displayspeed};
	}
public static boolean istalking() {
	return SuperGattCallback.dotalk||Natives.gettouchtalk()||Natives.speakmessages()||Natives.speakalarms();
	}
public static void config(MainActivity context) {
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
	seplabel.setPadding(pad*3,0,0,0);
	var speeds=slider(context,curspeed);

	var pitchs=slider(context,curpitch);
	var cancel=getbutton(context,R.string.cancel);
	var helpview=getbutton(context,R.string.helpname);
	helpview.setOnClickListener(v-> help.help(R.string.talkhelp,context));

	var save=getbutton(context,R.string.save);
	var width= GlucoseCurve.getwidth();
	var speedlabel=getlabel(context,context.getString(R.string.speed));
	speedlabel.setPadding(0,0,0,0);
	var pitchlabel=getlabel(context,context.getString(R.string.pitch));
	pitchlabel.setPadding(0,pad*5,0,0);
	var voicelabel=getlabel(context,context.getString(R.string.talker));
       var active=getcheckbox(context,R.string.speakglucose, SuperGattCallback.dotalk);
	active.setPadding(0,0,pad*3,0);

	var test=getbutton(context,context.getString(R.string.test));
			
	var spin= spinner!=null?spinner:((android.os.Build.VERSION.SDK_INT >= minandroid)? (spinner=new Spinner(context)):null);

	int[] spinpos={-1};
	View[]  firstrow;
	if(android.os.Build.VERSION.SDK_INT >= minandroid) { 
		spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			@Override
			public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
				Log.i(LOG_ID,"onItemSelected "+position);
				
				spinpos[0]=position;
				}
			@Override
			public  void onNothingSelected (AdapterView<?> parent) {

			} });
		avoidSpinnerDropdownFocus(spin);
		spin.setAdapter(new RangeAdapter<Voice>(voiceChoice, context, voice -> {
				return voice.getName();
				}));
		Log.i(LOG_ID,"voicepos="+voicepos);
		if(voicepos>=0&&voicepos<voiceChoice.size())
			spin.setSelection(voicepos);
		spinpos[0]=-1;
		 firstrow=new View[]{active,seplabel,separation,voicelabel,spin};
		}
	else {
		var space=new Space(context);
		space.setMinimumWidth((int)(width*0.4));
		 firstrow=new View[]{active,seplabel,separation,space};
		 }
	var touchtalk= getcheckbox(context,context.getString(R.string.talk_touch), Natives.gettouchtalk());
	var speakmessages= getcheckbox(context,context.getString(R.string.speakmessages), Natives.speakmessages());
	var speakalarms= getcheckbox(context,context.getString(R.string.speakalarms), Natives.speakalarms());
	var secondrow=new View[]{touchtalk, speakmessages, speakalarms };
	var layout=new Layout(context,(l,w,h)-> {
//		if(width>w) l.setX((width-w)/2);
		return new int[] {w,h};
		},firstrow,secondrow,new View[]{speedlabel},new View[]{speeds[1]}, new View[]{speeds[0]},new View[]{pitchlabel},new View[]{pitchs[1]}, new View[]{pitchs[0]}, new View[]{cancel,helpview,test,save});

      //layout.setBackgroundResource(R.drawable.dialogbackground);
	layout.setBackgroundColor( Applic.backgroundcolor);
	context.setonback(()-> { 
		tk.glucodata.help.hidekeyboard(context);
		removeContentView(layout); 
		spinner=null;
		if(Menus.on)
			Menus.show(context);
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
				}
			var speedstr=((EditText)speeds[1]).getText().toString();
			if(speedstr != null) {
				curspeed = str2float(speedstr);
				Log.i(LOG_ID,"speedstr: "+speedstr+" "+curspeed);
				}
			var pitchstr=((EditText)pitchs[1]).getText().toString();
			if(pitchstr != null) {
				curpitch = str2float(pitchstr);
				Log.i(LOG_ID,"pitchstr: "+pitchstr+" "+curpitch);
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
				talk.speak(say);
				return;
				}
			}
		playstring=say;
		SuperGattCallback.newtalker(context);
		});

	context.addContentView(layout, new ViewGroup.LayoutParams(MATCH_PARENT, WRAP_CONTENT));

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
