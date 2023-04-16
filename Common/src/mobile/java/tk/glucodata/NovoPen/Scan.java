package tk.glucodata.NovoPen;

import static android.graphics.Typeface.BOLD;
import static android.graphics.Typeface.DEFAULT_BOLD;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Dialogs.fdatename;
import static tk.glucodata.Natives.novopentype;
import static tk.glucodata.Natives.savenovopen;
import static tk.glucodata.Natives.setnovopenttimeandtype;
import static tk.glucodata.Natives.showbytes;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.ScanNfcV.failure;
import static tk.glucodata.ScanNfcV.getvibrator;
import static tk.glucodata.ScanNfcV.startvibration;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.nfc.Tag;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Space;
import android.widget.Spinner;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Locale;

import tk.glucodata.NovoPen.opennov.OpContext;
import tk.glucodata.NovoPen.opennov.OpenNov;

import tk.glucodata.Applic;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.LabelAdapter;
import tk.glucodata.Layout;
import tk.glucodata.Log;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;

//import android.nfc.Tag;

public class Scan {
	static final private String LOG_ID="Scan";
	private static long nexttime=0L;
	static public void onTag(MainActivity act, Tag tag) {
		showbytes("onTag", tag.getId());
		var now = System.currentTimeMillis();
		if (now > nexttime) {
			var vibrator= getvibrator(act);
		 	startvibration(vibrator);
			nexttime = now + 4000;
			var openNov = new OpenNov();
			var res = openNov.processTag(tag);
			vibrator.cancel();
			if (res == null) {
				failure(vibrator);
				final var failread=act.getString(R.string.penfailed);
				Log.d(LOG_ID, failread);
				nexttime = 0L;
				Applic.Toaster(failread);
			} else {
				Applic.RunOnUiThread(()->setInsulin(act,res));
			}
		}
	}

private static void earlytimeconfirmation(MainActivity act) {
        AlertDialog.Builder builder = new AlertDialog.Builder(act);
        builder.setTitle("To get older values you have to scan again").
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                    }
                }) .show();
	}
private static void changeTypeconfirmation(MainActivity act,String type,Runnable save) {
        AlertDialog.Builder builder = new AlertDialog.Builder(act);
        builder.setTitle(act.getString(R.string.changeinsulin)+type+"?").
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
		    	save.run();
                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show();
	}
static  public final DateFormat fhourmin=             new SimpleDateFormat("HH:mm", Locale.US);

static void setInsulin(MainActivity context, OpContext op) {
	if(op.specification==null) {
		Log.e(LOG_ID,"op.specification==null");
		final var failread=context.getString(R.string.penfailed);
		Applic.Toaster(failread);
		return;
		}
	if(op.eventReport==null) {
		Log.e(LOG_ID,"op.eventReport==null");
		final var failread=context.getString(R.string.penfailed);
		Applic.Toaster(failread);
		return;
		}
	if(op.eventReport.doses==null){
		Log.e(LOG_ID,"op.eventReport.doses==null");
		final var failread=context.getString(R.string.penfailed);

		Applic.Toaster(failread);
		return;
		}

	String serial=op.specification.getSerial();
	final long typetime =novopentype(serial);
	int type;
	long lasttime;
	if(typetime!=-1L) {
		type=(int)(typetime>>32);
		long sectime=(typetime&0xFFFFFFFFL);
		lasttime=sectime*1000L;
		final String datestr=fdatename.format(lasttime)      ;
		Log.i(LOG_ID,"type= "+type+" Last time: "+String.format("typetime=%X seconds=%X %d",typetime,sectime,sectime)+ " "+datestr);
		}
	else {
		type=-1;
		lasttime=0L;
		}

        Spinner spinner=new Spinner(context);
	final int minheight= GlucoseCurve.dpToPx(48);
	spinner.setMinimumHeight(minheight);
	avoidSpinnerDropdownFocus(spinner);
	int[] selected={type<0?0:type};
	final var labels=Natives.getLabels();
	LabelAdapter<String> numspinadapt=new LabelAdapter<String>(context,labels,1);
        spinner.setAdapter(numspinadapt);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
		@Override
		public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
			if(position>=0&&position<labels.size()) {
				selected[0]=position;
				Log.i(LOG_ID, "selected " + labels.get(position));
				}
			else
				Log.i(LOG_ID,"selected "+position);
			}
		@Override
		public  void onNothingSelected (AdapterView<?> parent) {

		} });
	 spinner.setSelection(selected[0]);
	float density=GlucoseCurve.metrics.density;
	int laypad=(int)(density*4.0);

//	var message=getlabel(context,context.getString(R.string.receiveddoses));
//	message.setPadding(laypad,0,laypad*2,0);

	var after=getlabel(context,context.getString(R.string.dosesprior));
	after.setPadding((int)(density*5.0f),laypad*2,laypad*3,0);
	var label=getlabel(context,"PEN"+serial);
	label.setTypeface(DEFAULT_BOLD,BOLD);
//	label.setPadding(laypad,0,laypad,0);
        var cancel=getbutton(context,R.string.cancel);
		var typestr=getlabel(context,context.getString(R.string.type));
	typestr.setPadding(laypad*2,0,0,laypad*5);
	var ok=getbutton(context, R.string.save);
	long[] newtime={lasttime};
//        Date dat = new Date(lasttime);
final	var datebutton=getbutton(context, DateFormat.getDateInstance(DateFormat.DEFAULT).format(lasttime));
//        datebutton.setText(); timebutton.setText(DateFormat.getTimeInstance(DateFormat.SHORT).format(dat));
	var cal = Calendar.getInstance();
        datebutton.setOnClickListener(
                v -> { 
			context.getnumberview().getdateviewal(context,newtime[0], (year,month,day)-> {
		     cal.set(Calendar.YEAR,year);
		     cal.set(Calendar.MONTH,month);
		     cal.set(Calendar.DAY_OF_MONTH,day);
		     long newmsec= cal.getTimeInMillis();
		     newtime[0]=newmsec;
			datebutton.setText(DateFormat.getDateInstance(DateFormat.DEFAULT).format(newmsec));
			});

		});	

          cal.setTimeInMillis(newtime[0]);
	  int[] hour={cal.get(Calendar.HOUR_OF_DAY)};
	int[]  min={cal.get(Calendar.MINUTE)};
	var timebutton=getbutton(context,  String.format(Locale.US,"%02d:%02d",hour[0],min[0] ));

	timebutton.setOnClickListener(v-> {
			context.getnumberview().gettimepicker(context,hour[0], min[0], (h,m) -> {
				hour[0]=h;
				min[0]=m;
				cal.set(Calendar.HOUR_OF_DAY,h);
				cal.set(Calendar.MINUTE,m);
			         newtime[0]= cal.getTimeInMillis();
				timebutton.setText(String.format(Locale.US,"%02d:%02d",h,m));
			   });});
//	var date= main.getnumberview().
    	var  layout=new Layout(context,(x,w,h)->{
//			var height=GlucoseCurve.getheight();
			var width=GlucoseCurve.getwidth();
			x.setX((width-w)/2);
			x.setY(0);
			return new int[] {w,h};
	},new View[]{label},new View[]{after},new View[]{datebutton,timebutton},new View[]{typestr,spinner},new View[]{ok,cancel});
        layout.setBackgroundColor(Applic.backgroundcolor);
	int pad=(int)(10.0*density);
	layout.setPadding(pad,pad,pad,(int)(density*14.0));

	context.setonback(() -> removeContentView(layout) );
	ok.setOnClickListener(v -> {
			var doses=op.eventReport.doses;
			int ty=selected[0];
			Runnable saveall=()-> {
				int nr=0;
				int lastdose=doses.size()-1;
				for(int d=0;d<=lastdose;++d) {
					var dose=doses.get(d);
					int back=savenovopen(dose.referencetime,serial,ty,dose.rawdoses,d==lastdose);
					if(back<0)  {
						Applic.Toaster(context.getString(R.string.wentwrong));
						}
					else
						nr+=back;
					}
				Applic.Toaster(nr+(nr==1?context.getString(R.string.dosis):context.getString(R.string.doses))+context.getString(R.string.saved));
				if(nr>0) context.requestRender();
				context.doonback();
				};
			Runnable testtype=() -> {
				if(ty==type) {
					saveall.run();
					}
				else {
					changeTypeconfirmation(context,labels.get(ty), saveall);
					}
				};
			if(lasttime==newtime[0]) {
				testtype.run();
				}
			else {
				setnovopenttimeandtype(newtime[0],ty,serial);
				if(lasttime<newtime[0]) {
					testtype.run();
					}
				else {
					context.doonback();
					earlytimeconfirmation(context);
					}
				}
			}
			);
	cancel.setOnClickListener(v -> context.doonback());
        context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));

	}
}
