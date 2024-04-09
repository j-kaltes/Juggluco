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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */



package tk.glucodata;
/*
long[] constatchange={0L,0L};
int constatstatus=-1;
long[] wrotepass={0L,0L};
long[] charcha={0L,0L};
@Override 
public void onCharacteristicChanged(BluetoothGatt bluetoothGatt, BluetoothGattCharacteristic bluetoothGattCharacteristic) {


Last success:		Last failure:	 Fail Info
*/

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Paint;
import android.os.Build;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.HorizontalScrollView;
import android.widget.Spinner;
import android.widget.TextView;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;


import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_BALANCED;
import static android.bluetooth.BluetoothGatt.CONNECTION_PRIORITY_HIGH;
import static android.graphics.Color.BLACK;
import static android.graphics.Color.BLUE;
import static android.graphics.Color.RED;
import static android.graphics.Color.WHITE;
import static android.graphics.Color.YELLOW;
import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.help.help;
import static tk.glucodata.settings.Settings.removeContentView;

class bluediag {

static  boolean returntoblue=false;
final static private String LOG_ID="bluediag";
private static final DateFormat fname=             new SimpleDateFormat("MM-dd HH:mm:ss", Locale.US );
public static String datestr(long tim) {
	return fname.format(tim);
	}
//View view ;
//int selected=0;

RangeAdapter<SuperGattCallback> adap;
Spinner spin=null;
TextView[] contimes;
TextView constatus;
TextView streaming;
TextView address;
TextView starttimeV;
TextView rssiview;
//Button forget;
//Button reenable;
Button info;
void setrow(long[] times, TextView[]  timeviews, TextView info) {
	for(int i=0;i<2;i++) {
		long tim= times[i];
		TextView text= timeviews[i];
		if(tim!=0L) {
			text.setText(datestr(tim));
			if(tim<times[(~i)&1]) {
				text.setPaintFlags(text.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
				if(i==1) 
					info.setPaintFlags(text.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
				}
				
			else {
				text.setPaintFlags(text.getPaintFlags() & (~Paint.STRIKE_THRU_TEXT_FLAG));
				if(i==1)
					info.setPaintFlags(text.getPaintFlags() & (~Paint.STRIKE_THRU_TEXT_FLAG));
				}
			}
		else {
			text.setText("");
			}
		}
	}


public static void showsensorinfo(String text,MainActivity act) {
		help.basehelp(text,act,xzy->{
		}, (l,w,h)-> {
			var height=GlucoseCurve.getheight();
			var width=GlucoseCurve.getwidth();
			if(height>h)
				l.setY((height-h)/2);
			if(width>w)
				l.setX((width-w)/2);
			return new int[] {w,h};
			}, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
		}

void showinfo(final SuperGattCallback gatt,MainActivity act) {
	starttimeV.setText(datestr(gatt.starttime));
	if(gatt.streamingEnabled() )
		streaming.setText(" Streaming enabled");
	else
		streaming.setText(" Streaming not enabled");
	
/*	var visi=gatt.sensorgen==3?INVISIBLE:VISIBLE;
	if(!isWearable) {
		if(reenable!=null)
			reenable.setVisibility(visi);
		} */
	final int rssi=gatt.readrssi;
	if(rssi<0) {
		rssiview.setText("Rssi = "+rssi);
		}
	else
		rssiview.setText(""); 
		 
/*	if(forget!=null)  {
		forget.setVisibility(visi);
		if(gatt.sensorgen!=3) {
			forget.setOnClickListener(v-> {
				gatt.searchforDeviceAddress();
				SensorBluetooth.startscan();
				 act.doonback();
				 new bluediag(act);

				});
			}
		}
		*/

	address.setText(gatt.mActiveDeviceAddress == null?"Address unknown":gatt.mActiveDeviceAddress);
	if(gatt.sensorgen == 2) {
//		address.setBackgroundColor(RED); address.setTextColor(BLACK);
		address.setTextColor(RED);
		}
	else {
//		address.setBackgroundColor(BLUE); address.setTextColor(WHITE);
		address.setTextColor(YELLOW);
		}
	constatus.setText(gatt.constatstatus>=0?("Status="+gatt.constatstatus):"");
	setrow(gatt.constatchange,contimes,constatus);
	keyinfo.setText(gatt.handshake);
	setrow(gatt.wrotepass,keytimes,keyinfo);
	setrow(gatt.charcha,glucosetimes,glucoseinfo);

	Log.i(LOG_ID,"info.setVisibility(VISIBLE);");
	info.setVisibility(VISIBLE);
	info.setOnClickListener(v->  showsensorinfo(gatt.getinfo(),(MainActivity) info.getContext()));
	}
TextView[] keytimes; TextView keyinfo;
TextView[] glucosetimes; TextView glucoseinfo;
TextView bluestate;
private BluetoothAdapter mBluetoothAdapter=null;
//boolean setwakelock=false;
CheckBox usebluetooth;
CheckBox priority,streamhistory;
Button locationpermission;
TextView scanview;
MainActivity activity;
static private int gattselected=0;

void confirmFinish(SuperGattCallback gat) {
        AlertDialog.Builder builder = new AlertDialog.Builder(activity);
	String serial= gat.SerialNumber;
        builder.setTitle(serial).
	 setMessage(R.string.finishsensormessage).
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
				gat.finishSensor();
				 SensorBluetooth.sensorEnded(serial);
				 activity.requestRender();
				 activity.doonback();
				new bluediag(activity);

                    		}
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {

            }
        }).show().setCanceledOnTouchOutside(false);
	}

void	setadapter(Activity act,	final ArrayList<SuperGattCallback> gatts) {
	adap = new RangeAdapter<>(gatts, act, gatt -> {
		if (gatt != null && gatt.SerialNumber != null)
			return gatt.SerialNumber;
		return "Error";
	});
	spin.setAdapter(adap);
}



bluediag(MainActivity act) {
	activity=act;
	BluetoothManager mBluetoothManager = (BluetoothManager) act.getSystemService(Context.BLUETOOTH_SERVICE);
        if(mBluetoothManager  != null) {
            mBluetoothAdapter = mBluetoothManager.getAdapter();
	      if(mBluetoothAdapter ==null) {
			Log.e(LOG_ID,"mBluetoothManager.getAdapter()==null");
			return;
			}
	    }
   else {

      		Log.e(LOG_ID,"act.getSystemService(Context.BLUETOOTH_SERVICE)==null");
		return;

	   	}
	LayoutInflater flater= LayoutInflater.from(act);
	View view = flater.inflate(R.layout.bluesensor, null, false);

	scanview=view.findViewById(R.id.scan);
	starttimeV=view.findViewById(R.id.stage);
	rssiview=view.findViewById(R.id.rssi);
	rssiview.setPadding(0,0,0,0);
	CheckBox android13=view.findViewById(R.id.android13);
	android13.setChecked( SuperGattCallback.autoconnect);
	android13.setOnCheckedChangeListener( (buttonView,  isChecked) -> { SensorBluetooth.setAutoconnect(isChecked); });
	info=view.findViewById(R.id.info);
	Log.i(LOG_ID,"info.setVisibility(INVISIBLE);");
	info.setVisibility(INVISIBLE);
	int width2=GlucoseCurve.getwidth();
	HorizontalScrollView addscroll2=null;
	if(isWearable) {
		HorizontalScrollView scroll= view.findViewById(R.id.background);
		scroll.setSmoothScrollingEnabled(false);
              scroll.setVerticalScrollBarEnabled(false);
                scroll.setHorizontalScrollBarEnabled(false);
		int height=GlucoseCurve.getheight();
		scroll.setMinimumHeight(height);
		Log.i(LOG_ID,"height="+height);
		}
	else {
		measuredgrid grid=view.findViewById(R.id.grid);
		final var addscroll= new HorizontalScrollView(act);
		addscroll.addView(grid);
		addscroll.setSmoothScrollingEnabled(false);
	      addscroll.setVerticalScrollBarEnabled(false);
		addscroll.setHorizontalScrollBarEnabled(false);
		int heightU=GlucoseCurve.getheight();
		addscroll.setMinimumHeight(heightU);
		grid.setmeasure((l,w,h)-> {
			int height=GlucoseCurve.getheight();
			int width=GlucoseCurve.getwidth();
			int y= height>h?((height-h)/2):0;
			addscroll.setY(y);
			int x=(width>w)?((width-w)/2):0;
			addscroll.setX(x);
		}); 
		addscroll2=addscroll;

	}

	View showview=addscroll2!=null?addscroll2:view;

		final ArrayList<SuperGattCallback> gatts=SensorBluetooth.mygatts();
	priority=view.findViewById(R.id.priority);
	streamhistory=view.findViewById(R.id.streamhistory);
if(!isWearable) {
	Button finish = view.findViewById(R.id.finish);
	if (gatts != null && gatts.size() > 0) {
		finish.setOnClickListener(v -> {
			if (gatts != null && gatts.size() > 0) {
				if (gattselected >= gatts.size()) {
					Log.i(LOG_ID, "show: gattselected=" + gattselected);
					gattselected = 0;
					return;
				}
				var gat = gatts.get(gattselected);
				confirmFinish(gat);
			}
		});
	}
	else {

		Log.i(LOG_ID,"finish.setVisibility(GONE);");
		finish.setVisibility(GONE);
	}
}
	if (gatts == null || gatts.size()== 0) {
		priority.setVisibility(GONE);
		}
	if(!Natives.getusebluetooth()) {
		streamhistory.setVisibility(GONE);
		}
	contimes=new TextView[]{view.findViewById(R.id.consuccess) , view.findViewById(R.id.confail)};
	constatus=view.findViewById(R.id.constatus);
	constatus.setTextIsSelectable(true);
	streaming=view.findViewById(R.id.streaming);
	address=view.findViewById(R.id.deviceaddress);
	keytimes=new TextView[]{view.findViewById(R.id.keysuccess) , view.findViewById(R.id.keyfailure)}; keyinfo=view.findViewById(R.id.keyinfo);
	keyinfo.setTextIsSelectable(true);
	glucosetimes=new TextView[]{view.findViewById(R.id.glucosesuccess) , view.findViewById(R.id.glucosefailure)}; glucoseinfo=view.findViewById(R.id.glucoseinfo);
	bluestate=view.findViewById(R.id.bluestate);
	usebluetooth=view.findViewById(R.id.usebluetooth);
	/*
	wakelock=view.findViewById(R.id.wakelock);
	wakelock.setOnCheckedChangeListener(
		 (buttonView,  isChecked) -> {
		 	if(!setwakelock) {
				keeprunning.setWakelock(isChecked);
				}
		 	}
		 ); */
	usebluetooth.setOnCheckedChangeListener(
		 (buttonView,  isChecked) -> {
		 	Log.i(LOG_ID,"usebluetooth "+isChecked);
			 final boolean blueused = Natives.getusebluetooth();
			 if (blueused != usebluetooth.isChecked()) {
				 act.setbluetoothmain( !blueused);
				 act.requestRender();
				 act.doonback();
				new bluediag(act);
			 }
		 }
		 );


	streamhistory.setOnCheckedChangeListener( (buttonView,  isChecked) -> Natives.setStreamHistory(isChecked) );
     if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
	priority.setOnCheckedChangeListener(
                 (buttonView,  isChecked) -> {
			 final boolean priorityused = Natives.getpriority();
			 if(priorityused != isChecked) {
			    setpriorities(isChecked,gatts);
			    }
		 }
		 );
		}
	else  {
		Log.i(LOG_ID,"priority.setVisibility(INVISIBLE);");
	   priority.setVisibility(INVISIBLE);
	   }
	boolean hasperm=Build.VERSION.SDK_INT < 23||Applic.noPermissions(act).length==0;
	if(!isWearable)  {
		locationpermission=view.findViewById(R.id.locationpermission);
		if(hasperm)   {
			Log.i(LOG_ID,"locationpermission.setVisibility(GONE);");
			locationpermission.setVisibility(GONE);
			}
		else {
			locationpermission.setOnClickListener(v-> {
				var noperm=Applic.noPermissions(act);
				if(noperm.length==0) {
					Log.i(LOG_ID,"locationpermission.setVisibility(GONE);");
					locationpermission.setVisibility(GONE);
					}

				else  {
					returntoblue=true;
					act.doonback();
					act.requestPermissions(noperm, act.LOCATION_PERMISSION_REQUEST_CODE);
					}
				});
			}
		}
	spin=view.findViewById(R.id.sensors);
	boolean[] first={true};
	spin.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
		@Override
		public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
			Log.i(LOG_ID,"onItemSelected");
			try {
				if (first[0]) {
					first[0] = false;
					spin.setSelection(gattselected);
					}
			else {		
					
				if (gatts != null && gatts.size() > position) {
					gattselected = position;
					Log.i(LOG_ID, "onItemSelected: gattselected=" + gattselected);
					SuperGattCallback gatt = gatts.get(gattselected);
					showinfo(gatt, act);
				}
				}
			}
				catch(Throwable e) {
					Log.stack(LOG_ID,e);
					}

		}
		@Override
		public  void onNothingSelected (AdapterView<?> parent) {

		} });
	avoidSpinnerDropdownFocus(spin);
	if(gatts!=null) {
	      setadapter(act,gatts);
		}
	if(!isWearable) {
		 Button help=!isWearable?view.findViewById(R.id.help):null;
		help.setOnClickListener(v-> help(R.string.sensorhelp,act));
		 Button background=view.findViewById(R.id.background);
		if(android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
			background.setOnClickListener(v-> Battery.batteryscreen(act,showview));
			}
		else  {
		Log.i(LOG_ID,"background.setVisibility(GONE);");
		    background.setVisibility(GONE);
		    }
		}


	 Button close=view.findViewById(R.id.close);
	 close.setOnClickListener(v-> act.doonback());

      view.setBackgroundColor( Applic.backgroundcolor);
	show(act,showview);
	act.addContentView(showview, new ViewGroup.LayoutParams( WRAP_CONTENT, WRAP_CONTENT));
	var scheduled=Applic.scheduler.scheduleAtFixedRate( ()-> {
       Log.i(LOG_ID,"scheduled");
	    act.runOnUiThread( ()-> { 
         if(gatts!=null&&gatts.size()>0) {
            if(gattselected>= gatts.size()) {
               Log.i(LOG_ID,"show: gattselected="+ gattselected);
               gattselected=0;
               }
            showinfo(gatts.get(gattselected),act);
            }

	    });},29,29, TimeUnit.SECONDS);
	act.setonback(() -> {
		   Log.i(LOG_ID,"onback");
			scheduled.cancel(false);
			act.setfineres(null);
			removeContentView(showview);
			 if(Menus.on) {
				Menus.show(act);
				}


			});


    }

final static class Pair{
 public	long key;
public	String value;
        public Pair(long key,  String value){
            this.key = key;
            this.value = value;
        }
public	long getKey() {
		return key;
		}
    };
static void put(List<Pair> l,long key,String val) {
	l.add(new Pair(key,val));	
	}
static class onkey implements Comparator<Pair> {
    public int compare(Pair a, Pair b)
    {
        return (int)(a.key - b.key);
    }
}
/*
static void test() {

	final List<Pair> messages = new ArrayList<>();
	put(messages,10L,"hallo");
	put(messages,0L,"one 0");
	put(messages,5L,"niets");
	put(messages,50L,"alles");
	put(messages,0L,"two 0");
	Collections.sort(messages, new onkey());
	for(Pair el:messages) {
		System.out.println("key="+el.key+" value="+		el.value);
	}

}*/
void showall() {
Log.i(LOG_ID,"showall");
//	test();
	SensorBluetooth  blue=SensorBluetooth.blueone;
	if(blue!=null&&blue.scantime!=0L) {
		final List<Pair> messages = new ArrayList<>();
		put(messages,blue.scantime,": Start search for sensors\n");
//		if(blue.unknownfound>0L) put(messages,blue.unknownfound,": Skip "+blue.unknownname+"\n");

		final ArrayList<SuperGattCallback> gatts=SensorBluetooth.mygatts();
		if(gatts==null) {
			Log.i(LOG_ID,"showall gatts==null");
			return;
			}
		for(SuperGattCallback gatt:gatts) {
			if(gatt.foundtime>0L)
				put(messages,gatt.foundtime,": Found "+gatt.SerialNumber +"\n");
			}
		if(blue.scantimeouttime>0L)
			put(messages,blue.scantimeouttime, ": timeout\n");
		if(blue.stopscantime>0L)
			put(messages,blue.stopscantime, ": Stop searching\n");
		Collections.sort(messages, new onkey());
		StringBuilder builder= new StringBuilder();
		    for (Pair entry : messages) {
				builder.append(datestr(entry.key));
				builder.append(entry.value);
			}
		
		builder.deleteCharAt(builder.length()-1);
		scanview.setText(builder);
		Log.i(LOG_ID,"scanview.setVisibility(VISIBLE);");
		scanview.setVisibility(VISIBLE);
		}
	else  {
			Log.i(LOG_ID,"scanview.setVisibility(GONE);");
			scanview.setVisibility(GONE);
			}
	if(!isWearable) {
		activity.setfineres(()-> {

		if( Build.VERSION.SDK_INT < 23|| Applic.noPermissions(activity).length==0) {
			Log.i(LOG_ID,"locationpermissin.setVisibility(GONE);");
			locationpermission.setVisibility(GONE);
			}
		});
		}
	bluestate.setText( mBluetoothAdapter==null?activity.getString(R.string.nobluetooth):(mBluetoothAdapter.isEnabled()?activity.getString(R.string.bluetoothenabled): activity.getString(R.string.bluetoothdisabled)));
        usebluetooth.setChecked(Natives.getusebluetooth());
	/*setwakelock=true;
        wakelock.setChecked(Natives.getwakelock());
	setwakelock=false;
	*/
        priority.setChecked(Natives.getpriority());
	streamhistory.setChecked(Natives.getStreamHistory( ));
	if(!isWearable) {
		if( Build.VERSION.SDK_INT < 23|| Applic.noPermissions(activity).length==0) {
			Log.i(LOG_ID,"locationpermissin.setVisibility(GONE);");
			locationpermission.setVisibility(GONE);
			}
		}
	Log.i(LOG_ID,"showall end");
	}

	
private void show(MainActivity act,View view) {
	if(spin!=null) {
		SensorBluetooth.updateDevices() ;
		final ArrayList<SuperGattCallback> gatts=SensorBluetooth.mygatts();
		setadapter(activity,gatts);
		if(gatts!=null&&gatts.size()>0) {
			if(gattselected>= gatts.size()) {
				Log.i(LOG_ID,"show: gattselected="+ gattselected);
				gattselected=0;
				}
			avoidSpinnerDropdownFocus(spin);
			showinfo(gatts.get(gattselected),act);
			}
		}
	Log.i(LOG_ID,"view.setVisibility(VISIBLE);");
	view.setVisibility(VISIBLE);

	showall();	
	}

  private static  void setpriorities(boolean isChecked,ArrayList<SuperGattCallback> gatts) {
	  if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
		  Natives.setpriority(isChecked);
		  if(gatts!=null) {
			  final int use_priority = isChecked ? CONNECTION_PRIORITY_HIGH : CONNECTION_PRIORITY_BALANCED;
			  for (SuperGattCallback g : gatts) {
				  try {
					  var ga = g.mBluetoothGatt;
					  if (ga != null)
						  ga.requestConnectionPriority(use_priority);
				  } catch (Throwable th) {
					  Log.stack(LOG_ID, "setpriorities", th);
				  }
			  }
		  }
	  }

  }
};



