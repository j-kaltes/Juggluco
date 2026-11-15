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


package tk.glucodata;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Spanned;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;

import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import static tk.glucodata.util.getradiobuttonId;

import androidx.appcompat.app.AlertDialog;

import com.garmin.android.connectiq.IQDevice;
import com.garmin.android.connectiq.ConnectIQ.IQMessageStatus;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import tk.glucodata.nums.AllData;
import tk.glucodata.settings.Shortcuts;

import android.text.method.ScrollingMovementMethod;


import static android.text.InputType.TYPE_NULL;
import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.help;
import static tk.glucodata.help.helplight;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.timestring;
import static tk.glucodata.Applic.backgroundcolor;

//import static tk.glucodata.GlucoseCurve.width;
//import static tk.glucodata.GlucoseCurve.height;

class GarminStatus {
	Spinner spinner;
	//s/^[ 	]*\([^=]*\)=new.*/TextView \1;/g
	TextView sdkreadyview;
	TextView restview,connection;
	//	TextView GarminStatusstr;
	CheckDirectionBox glucose;
	RadioGroup sensorConnection;
	CheckDirectionBox active;
	RadioButton automaticTransport;
	RadioButton garminConnectTransport;
	RadioButton directTransport;
	AllData alldata;
	Button next;
	Button sync;
	CheckDirectionBox numbersDevice;
	long selectedPeerId=-1L;
	boolean updatingSpinner=false;
	boolean closed=false;
	List<AppRow> shownApps=Collections.emptyList();
	private final Runnable refreshStatus=new Runnable() {
		@Override public void run() {
			if(closed || layout==null || layout.getParent()==null) return;
			show();
			layout.postDelayed(this,1000L);
		}
	};
	Layout layout;
	private static final String LOG_ID = "GarminStatus";

	private record AppRow(long peerId, String appId, String name, boolean connected, boolean installed) {}

	private static String displaystr(Context context,AppRow app) {
		int label=R.string.garmin_app_kerfstok;
/*		if(AllData.LIBRE2_GARMIN_APP_ID.equals(app.appId)) label=R.string.garmin_app_libre2;
		else */if(AllData.LIBRE3_GARMIN_APP_ID.equals(app.appId)) label=R.string.garmin_app_libre3;
		int status=app.connected?R.string.isconnected:R.string.isdisconnected;
		String appName=context.getString(label);
		if(!app.installed) appName=context.getString(R.string.garmin_app_unconfirmed,appName);
		return context.getString(R.string.garmin_app_device_row,appName,app.name,context.getString(status));
	}

	private AllData.GarminDeviceInfo selectedDevice(List<AllData.GarminDeviceInfo> devices) {
		for(var device:devices) if(device.id==selectedPeerId) return device;
		return null;
	}

	// AllData's English state values are lookup keys here, not displayed text.
	// Keep this presentation mapping compatible with the existing transport code.
	private static String communicationStatus(Context context,String state) {
		int text=R.string.garmin_status_unknown;
		if(state!=null) switch(state) {
			case "Kerfstok closed": text=R.string.garmin_status_closed; break;
			case "Recovering communication": text=R.string.garmin_status_recovering; break;
			case "Communication error": text=R.string.garmin_status_error; break;
			case "Connecting": text=R.string.garmin_status_connecting; break;
			case "Not connected": text=R.string.isdisconnected; break;
			case "Waiting for Kerfstok START": text=R.string.garmin_status_waiting_start; break;
			case "Connected; Kerfstok not yet ready": text=R.string.garmin_status_not_ready; break;
			case "Waiting for glucose acknowledgement": text=R.string.garmin_status_waiting_glucose; break;
			case "Waiting for history acknowledgement": text=R.string.garmin_status_waiting_history; break;
			case "Two-way communication confirmed": text=R.string.garmin_status_confirmed; break;
			case "Receiving; replies not yet confirmed": text=R.string.garmin_status_receiving; break;
			case "Sending; replies not yet confirmed": text=R.string.garmin_status_sending; break;
			case "Connected; communication not yet confirmed": text=R.string.garmin_status_unconfirmed; break;
		}
		return context.getString(text);
	}

	private static String transportResult(Context context,IQMessageStatus status) {
		int text;
		switch(status) {
			case SUCCESS: text=R.string.success; break;
			case FAILURE_DEVICE_NOT_CONNECTED: text=R.string.isdisconnected; break;
			case FAILURE_DURING_TRANSFER: text=R.string.garmin_transfer_failed; break;
			case FAILURE_INVALID_DEVICE: text=R.string.garmin_invalid_device; break;
			case FAILURE_INVALID_FORMAT: text=R.string.garmin_invalid_format; break;
			case FAILURE_MESSAGE_TOO_LARGE: text=R.string.garmin_message_too_large; break;
			case FAILURE_UNSUPPORTED_TYPE: text=R.string.garmin_unsupported_type; break;
			case FAILURE_UNKNOWN:
			default: text=R.string.garmin_unknown_failure; break;
		}
		return context.getString(text);
	}

	private static String lastError(Context context,String error) {
		if("No Kerfstok START acknowledgement".equals(error)) return context.getString(R.string.garmin_no_start_ack);
		if("No glucose acknowledgement".equals(error)) return context.getString(R.string.garmin_no_glucose_ack);
		// Other values are detailed transport diagnostics supplied by AllData.
		return error;
	}

	static private void setidview(MainActivity context, AllData alldata,View parent,View parentlayout) {
		EnableControls(parent,false);
		var idlabel = getlabel(context, context.getString(R.string.garmin_watch_app_id));

		var defaultapp = new CheckDirectionBox(context);
		defaultapp.setText(R.string.defaultname);
		var editid = new EditText(context);
		editid.setImeOptions(tk.glucodata.settings.Settings.editoptions);
		final String defaultid = Natives.getdefaultid();
		String garminid = Natives.getgarminid();
		editid.setText(garminid);
		var filter = new InputFilter() {
			@Override
			public CharSequence filter(CharSequence source,
									   int start,
									   int end,
									   Spanned dest,
									   int dstart,
									   int dend) {
				StringBuilder builder = new StringBuilder();
				for (int i = start; i < end; i++) {
					if (Character.digit(source.charAt(i), 16) == -1)
						return "";
					builder.append(Character.toUpperCase(source.charAt(i)));
				}
				return builder.toString();
			}
		};
		editid.setFilters(new InputFilter[]{filter});
		if (defaultid.equals(garminid)) {
			defaultapp.setChecked(true);
			editid.setInputType(TYPE_NULL);
		} else {
			defaultapp.setChecked(false);
//			editid.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD|InputType.TYPE_TEXT_FLAG_CAP_WORDS);
			editid.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
			//	editid.setKeyListener(DigitsKeyListener.getInstance("0123456789ABCDEF"));
		}

		defaultapp.setOnCheckedChangeListener(
				(buttonView, isChecked) -> {
					if (isChecked) {
						editid.setText(defaultid);
						editid.setInputType(TYPE_NULL);
						hidekeyboard(context);
						context.hideSystemUI();
					} else {
						//		editid.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD|InputType.TYPE_TEXT_FLAG_CAP_WORDS);
						editid.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
//					editid.setKeyListener(DigitsKeyListener.getInstance("0123456789ABCDEF"));
						editid.requestFocus();
						tk.glucodata.help.showkeyboard(context, editid);
					}

				});
		var Save = getbutton(context, R.string.save);
		var Cancel = getbutton(context, R.string.cancel);

		var layout = new Layout(context, (l, w, h) -> {
			return new int[]{w, h};
		}, new View[]{idlabel, defaultapp}, new View[]{editid}, new View[]{Cancel, Save});

		float density = GlucoseCurve.metrics.density;
		int laypad = (int) (density * 4.0);
		layout.setPadding(laypad * 2, laypad * 2, laypad * 2, laypad);
        layout.systembarMargins();

//	layout.setBackgroundResource(R.drawable.dialogbackground);
   layout.setBackgroundResource(R.drawable.dialogbackground);

    var  params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER_HORIZONTAL);

    context.addMyContentView(layout, params);
//		context.addMyContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
		context.setonback(() -> {
			EnableControls(parent,true);
			removeContentView(layout);
			context.hideSystemUI();
			hidekeyboard(context);

		});
		Cancel.setOnClickListener(
				v -> context.doonback()
                                );
		Save.setOnClickListener(
				v -> {
					boolean changed = false;
					if (defaultapp.isChecked()) {
						if (!defaultid.equals(garminid)) {
							changed = Natives.setgarminid(null);
						}
					} else {
						String id = editid.getText().toString();
						int idlen = id.length();
						if (idlen != 32) {
							Applic.argToaster(context, context.getString(R.string.garmin_id_length,idlen), Toast.LENGTH_SHORT);
							return;
						}
						if (!garminid.equals(id)) {
							if (!(changed = Natives.setgarminid(id))) {
								Applic.argToaster(context, context.getString(R.string.garmin_id_change_failed), Toast.LENGTH_SHORT);
								return;
							}

						}
					}
					if (changed)
						alldata.restartGarmin(context);
					context.doonback();
					context.doonback();
					context.doonback();
					new GarminStatus(context, alldata,parentlayout);

				});

	}

	public GarminStatus(MainActivity context, AllData alldata,View parentlayout) {

      context.themeLightBars();
   	//	EnableControls(parentlayout,false);
		this.alldata = alldata;
		// Pairing is performed in Android Bluetooth settings. Re-read bonded and
		// Garmin Connect devices whenever Garmin Status is opened so a newly paired
		// watch appears without restarting Juggluco.
		alldata.refreshGarminDevices(context);
		spinner = new Spinner(context);
		spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			@Override
			public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
				if(updatingSpinner || closed || position!=spinner.getSelectedItemPosition() ||
						position<0 || position>=shownApps.size()) return;
				var row=shownApps.get(position);
				selectedPeerId=row.peerId;
				if(!alldata.selectGarminStatusApp(context,row.peerId,row.appId)) {
					var current=selectedDevice(alldata.getGarminDeviceInfos());
//					if(current!=null && (current.libre3Direct || GarminLibre2.ownsPeer(row.peerId)))
					if(current!=null && current.libre3Direct)
						Applic.argToaster(context,context.getString(R.string.garmin_app_sensor_assigned),Toast.LENGTH_SHORT);
				}
				show();
			}

			@Override
			public void onNothingSelected(AdapterView<?> parent) { }
		});
		selectedPeerId=alldata.getNumbersDeviceId();
		avoidSpinnerDropdownFocus(spinner);

		sdkreadyview = new TextView(context);
		float density = GlucoseCurve.metrics.density;
		sdkreadyview.setPadding(0,0,(int)(density*10.0f),0);

		restview = new TextView(context);
        restview.setMovementMethod(new ScrollingMovementMethod());

        restview.setSingleLine(false);
        restview.setHorizontallyScrolling(false);
        restview.setEllipsize(null);
        restview.setLayoutParams(new ViewGroup.LayoutParams( ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));


        var method=getlabel(context,R.string.mirror_transport);
		sync = new Button(context);
		sync.setText(R.string.sync);
		sync.setOnClickListener(v -> alldata.sync(selectedPeerId));
		Button ok = new Button(context);
		ok.setText(R.string.closename);
		Button help = getbutton(context, R.string.helpname);
		help.setOnClickListener(v -> help(R.string.kerfstok, context));
		glucose = new CheckDirectionBox(context);
		glucose.setText(R.string.glucose);
		glucose.setChecked(false);
		glucose.setOnClickListener(v -> {
			if(!alldata.setGarminGlucose(context,selectedPeerId,glucose.isChecked())) {
				glucose.setChecked(false);
				Applic.argToaster(context,context.getString(R.string.garmin_select_device),Toast.LENGTH_SHORT);
				}
			show();
			});
      
		sensorConnection=new RadioGroup(context);
		var margins=Layout.getMargins(sensorConnection);
		margins.topMargin=(int)(density*8.0f);
		margins.rightMargin=(int)(density*8.0f);
		sensorConnection.setOrientation(RadioGroup.HORIZONTAL);
		var direct=getlabel(context,R.string.directsensor);
		if(direct.getText().length()==0) direct.setVisibility(GONE);
		connection=getlabel(context,R.string.connectionl);
		var phone=getradiobuttonId(context,R.string.phone,0);
		var watch=getradiobuttonId(context,R.string.watch,1);
		sensorConnection.addView(direct);
		sensorConnection.addView(phone);
		sensorConnection.addView(watch);
		phone.setOnClickListener(v -> selectSensorReceiver(context,false));
		watch.setOnClickListener(v -> selectSensorReceiver(context,true));

		active=getcheckbox(context,R.string.active,false);
		active.setOnClickListener(v -> {
			if(!alldata.setGarminActive(context,selectedPeerId,active.isChecked())) {
				active.setChecked(false);
				Applic.argToaster(context,context.getString(R.string.garmin_select_device),Toast.LENGTH_SHORT);
				}
			show();
			});
 		numbersDevice=getcheckbox(context,R.string.use_for_numbers,false);
		numbersDevice.setOnClickListener(v -> {
			if(selectedPeerId==-1L) {
				numbersDevice.setChecked(false);
				Applic.argToaster(context,context.getString(R.string.garmin_select_device),Toast.LENGTH_SHORT);
				return;
			}
			if(!numbersDevice.isChecked()) {
				if(!alldata.setNumbersDeviceEnabled(context,selectedPeerId,false))
					Applic.argToaster(context,context.getString(R.string.garmin_select_device),Toast.LENGTH_SHORT);
				show();
				return;
			}
			long oldId=alldata.getNumbersDeviceId();
			if(oldId==selectedPeerId) {
				show();
				return;
			}
			AllData.GarminDeviceInfo selected=null;
			for(AllData.GarminDeviceInfo info:alldata.getGarminDeviceInfos())
				if(info.id==selectedPeerId) { selected=info; break; }
			if(selected==null) {
				numbersDevice.setChecked(false);
				Applic.argToaster(context,context.getString(R.string.garmin_select_device),Toast.LENGTH_SHORT);
				return;
			}
			final AllData.GarminDeviceInfo target=selected;
			new AlertDialog.Builder(context)
					.setTitle(R.string.garmin_change_numbers_title)
					.setMessage(context.getString(R.string.garmin_change_numbers_message,
							alldata.getNumbersDeviceName(),target.name))
					.setNegativeButton(android.R.string.cancel,(dialog,which) -> {
						numbersDevice.setChecked(false);
						show();
					})
					.setPositiveButton(R.string.garmin_change,(dialog,which) -> {
						if(!alldata.setNumbersDevice(context,target.id))
							Applic.argToaster(context,context.getString(R.string.garmin_select_device),Toast.LENGTH_SHORT);
						show();
					})
					.setOnCancelListener(dialog -> {
						numbersDevice.setChecked(false);
						show();
					})
					.show();
			});

		RadioGroup transport=new RadioGroup(context);

		transport.setOrientation(RadioGroup.VERTICAL);

        var  marg2=new ViewGroup.MarginLayoutParams( ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        marg2.bottomMargin=(int)(density*8.0f);

        transport.setLayoutParams(marg2);

		automaticTransport=new RadioButton(context);
 		automaticTransport.setText(R.string.transport_automatic);
		garminConnectTransport=new RadioButton(context);
 		garminConnectTransport.setText(R.string.via_garmin);
		directTransport=new RadioButton(context);
		directTransport.setText(R.string.transport_bluetooth);
      RadioButton[] transportRadios={automaticTransport, garminConnectTransport,directTransport};
        transport.addView(method,new ViewGroup.LayoutParams(MATCH_PARENT,WRAP_CONTENT));
        for(var b: transportRadios)
            transport.addView(b,new ViewGroup.LayoutParams(MATCH_PARENT,WRAP_CONTENT));
        transport.addView(sensorConnection);
        transport.addView(connection);
		switch(alldata.getGarminTransportMode(context)) {
			case AllData.GARMIN_TRANSPORT_CONNECT: garminConnectTransport.setChecked(true); break;
			case AllData.GARMIN_TRANSPORT_DIRECT: directTransport.setChecked(true); break;
			default: automaticTransport.setChecked(true); break;
		}
		automaticTransport.setOnClickListener(v ->
			alldata.setGarminTransportMode(context,AllData.GARMIN_TRANSPORT_AUTOMATIC));
		garminConnectTransport.setOnClickListener(v ->
			alldata.setGarminTransportMode(context,AllData.GARMIN_TRANSPORT_CONNECT));
		directTransport.setOnClickListener(v ->
			alldata.setGarminTransportMode(context,AllData.GARMIN_TRANSPORT_DIRECT));
		next = new Button(context);
		next.setText(R.string.sendqueue);
		next.setOnClickListener(v -> alldata.nextmessage(selectedPeerId));
		Button reinit = new Button(context);
		reinit.setText(R.string.reinit);
		reinit.setOnClickListener(v -> alldata.reinit(context,selectedPeerId));
		Button config = getbutton(context, R.string.config);
		config.setOnClickListener(v ->  {
                        closed=true;
                        layout.removeCallbacks(refreshStatus);
                        kerfstokconfig(context,alldata,layout,parentlayout,selectedPeerId);
                        }
                        );

//		restview.setPadding(0,0,0,0);
		layout = new Layout(context, (l, w, h) -> {
			return new int[]{w, h};
		}, new View[]{spinner,sync,next,reinit,sdkreadyview}, new View[]{restview,transport},new View[]{help,config,active,glucose,numbersDevice,ok}
		).portraitLayout(new View[]{spinner}, new View[]{sync,reinit,next},new View[]{sdkreadyview},new View[]{transport},new View[]{restview},new View[]{active,glucose,numbersDevice}, new View[]{config,help, ok});
      layout.setBackgroundColor(backgroundcolor);
 //  layout.setBackgroundResource(R.drawable.dialogbackground);
		int laypad = (int) (density * 8.0);
//		layout.setPadding(laypad * 2, laypad * 2, laypad * 2, laypad*2);

       layout.systembarPadding((left,top,right,bottom)->new int[]{left+laypad,top,right+laypad,bottom});
//    var  params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER|Gravity.CENTER_HORIZONTAL);
//    var  params =    new FrameLayout.LayoutParams( MATCH_PARENT, MATCH_PARENT, 0);

    context.addMyContentView(layout,new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));

		ok.setOnClickListener(
				v -> {
				 context.doonback();
				}
			);
		context.setonback(() ->  {
                        closed=true;
                        layout.removeCallbacks(refreshStatus);
                       // EnableControls(parentlayout,true);
                        context.lightBars(!Natives.getInvertColors());
                        removeContentView(layout);
                        });
		show();
		layout.postDelayed(refreshStatus,1000L);
	}

	private void selectSensorReceiver(MainActivity context,boolean watch) {
		var info=selectedDevice(alldata.getGarminDeviceInfos());
		if(info!=null && info.active) {
        /*(
			if(AllData.LIBRE2_GARMIN_APP_ID.equals(info.appId)) {
				if(watch) GarminLibre2.assign(context,alldata,info.id);
				else GarminLibre2.returnToPhone(info.id);
			} else*/ if(AllData.LIBRE3_GARMIN_APP_ID.equals(info.appId)) {
				if(!alldata.setGarminLibre3Direct(context,info.id,watch)) {
					Applic.argToaster(context,context.getString(R.string.garmin_select_device),Toast.LENGTH_SHORT);
				} else if(watch && !GarminLibre3.switchCurrentSensor()) {
					alldata.setGarminLibre3Direct(context,info.id,false);
				}
			}
		}
		show();
	}

	private static List<AppRow> appRows(List<AllData.GarminDeviceInfo> devices) {
		var rows=new ArrayList<AppRow>();
		for(var device:devices) {
			for(String appId:device.installedApps)
				rows.add(new AppRow(device.id,appId,device.name,device.connected,true));
			// Keep the current app visible while discovery is pending or while
			// an assigned app is unavailable. Refreshing status cannot switch it.
			if(!device.installedApps.contains(device.appId))
				rows.add(new AppRow(device.id,device.appId,device.name,device.connected,false));
		}
		return rows;
	}

	private int selectedPosition(List<AppRow> rows,AllData.GarminDeviceInfo info) {
		if(info!=null) for(int i=0;i<rows.size();i++) {
			var row=rows.get(i);
			if(row.peerId==info.id && row.appId.equals(info.appId)) return i;
		}
		return 0;
	}

	public void show() {
		try {
			var devices=alldata.getGarminDeviceInfos();
			var info=selectedDevice(devices);
			if(info==null) {
				selectedPeerId=alldata.getNumbersDeviceId();
				info=selectedDevice(devices);
				if(info==null && !devices.isEmpty()) info=devices.get(0);
			}
			var rows=appRows(devices);
			int pos=selectedPosition(rows,info);
			updatingSpinner=true;
			if(!shownApps.equals(rows)) {
				var context=spinner.getContext();
				spinner.setAdapter(new RangeAdapter<>(rows,context,row -> displaystr(context,row)));
				shownApps=rows;
			}
			if(info!=null) {
				selectedPeerId=info.id;
				if(spinner.getSelectedItemPosition()!=pos) spinner.setSelection(pos);
			}
			updateDetails(info);
		} catch(Throwable e) {
			Log.stack(LOG_ID,"show",e);
		} finally {
			updatingSpinner=false;
		}
	}

	private void updateDetails(AllData.GarminDeviceInfo info) {
		int transportMode=alldata.getGarminTransportMode(spinner.getContext());
		automaticTransport.setChecked(transportMode==AllData.GARMIN_TRANSPORT_AUTOMATIC);
		garminConnectTransport.setChecked(transportMode==AllData.GARMIN_TRANSPORT_CONNECT);
		directTransport.setChecked(transportMode==AllData.GARMIN_TRANSPORT_DIRECT);
		active.setEnabled(info!=null);
		active.setChecked(info!=null && info.active);
		numbersDevice.setEnabled(info!=null);
		numbersDevice.setChecked(info!=null && info.numbers);
		Context context=spinner.getContext();
		if(info==null) sdkreadyview.setText(R.string.garmin_no_device);
		else sdkreadyview.setText(context.getString(R.string.garmin_connection_status,
				context.getString(info.direct?R.string.transport_bluetooth:R.string.garmin_connect)));
//				context.getString(info.connected?R.string.isconnected:R.string.isdisconnected)));
		StringBuilder builder=new StringBuilder();
		if(info!=null) {
			String none=context.getString(R.string.garmin_none);
			builder.append(context.getString(R.string.garmin_kerfstok_status,communicationStatus(context,info.communicationStatus))).append('\n');
			builder.append(context.getString(R.string.garmin_last_confirmed_reply,
					info.lastAcknowledged==0?none:timestring(info.lastAcknowledged))).append('\n');
			builder.append(context.getString(R.string.garmin_glucose_confirmed,
					info.lastGlucoseAcknowledged==0?none:timestring(info.lastGlucoseAcknowledged)));
			if(info.lastGlucoseAcknowledged!=0) {
				builder.append('\n').append(context.getString(R.string.garmin_glucose_sample,timestring(info.acknowledgedGlucoseTime*1000L)));
			}
			builder.append('\n').append(context.getString(R.string.garmin_glucose_ack,
					context.getString(info.timestampedGlucoseAck?R.string.garmin_ack_timestamped:R.string.garmin_ack_legacy)));
			builder.append('\n').append(context.getString(R.string.garmin_last_sent,info.lastSend==0?none:timestring(info.lastSend)));
			builder.append('\n').append(context.getString(R.string.garmin_last_received,info.lastReceived==0?none:timestring(info.lastReceived)));
			if(info.lastStatus!=null && info.lastStatusTime>=info.lastSend) {
				builder.append('\n').append(context.getString(R.string.garmin_transport_result,transportResult(context,info.lastStatus)));
			}
			if(info.lastError!=null) builder.append('\n').append(context.getString(R.string.garmin_last_error,lastError(context,info.lastError)));
            /*
			String assignment=GarminLibre2.assignmentStatus(context,info.id);
			if(assignment!=null) builder.append('\n').append(assignment);
            */
		}
		restview.setText(builder.toString());
//        restview.post(() -> Log.d(LOG_ID, "width=" + restview.getWidth() + " measuredWidth=" + restview.getMeasuredWidth()));
		// Glucose is a user preference, not a connection-status indicator.  Keep
		// it available for any known watch so the user can enable sending before
		// the watch connects; queued/latest glucose will be sent when it becomes
		// ready.
		glucose.setChecked(info!=null && info.glucose);
		glucose.setVisibility(info!=null?VISIBLE:GONE);
		glucose.setEnabled(info!=null);
//		boolean libre2=info!=null && AllData.LIBRE2_GARMIN_APP_ID.equals(info.appId);
		boolean libre3=info!=null && AllData.LIBRE3_GARMIN_APP_ID.equals(info.appId);
		boolean sensorApp= libre3;
		sensorConnection.check((libre3 && info.libre3Direct)?1:0);
		sensorConnection.setVisibility(sensorApp?VISIBLE:GONE);
		connection.setVisibility(sensorApp?VISIBLE:GONE);
		if(sensorApp) connection.setText(context.getString(R.string.connectionl)+" (Libre 3)");
		EnableControls(sensorConnection,sensorApp && info.active);
		connection.setEnabled(sensorApp && info.active);
		boolean selectedNumbers=info!=null && info.numbers;
		next.setVisibility(info!=null && info.active && selectedNumbers && alldata.waiting(selectedPeerId) && !alldata.isSending()?VISIBLE:GONE);
		sync.setVisibility(info!=null?VISIBLE:GONE);
		sync.setEnabled(info!=null && info.active && selectedNumbers);
		layout.setVisibility(VISIBLE);
	}

	private void kerfstokconfig(MainActivity context,AllData alldata,View parent,View parentlayout,long peerId) {
		EnableControls(parent,false);
		var setid = getbutton(context, R.string.garmin_app_id_button);

		Button apppresent = getbutton(context, ((AllData.appmissing < 0) ? context.getString(R.string.watchappinstalled) : context.getString(R.string.getkerfstok)));
		apppresent.setOnClickListener(v -> {
			final String url = "https://apps.garmin.com/en-US/apps/b6348ccc-86d8-4780-8013-d9e19fed5260";
			Uri uri = Uri.parse(url);
			Intent intent = new Intent(Intent.ACTION_VIEW, uri);
			try { 
				if (intent.resolveActivity(context.getPackageManager()) != null) {
					context.startActivity(intent);
					}
				}
			catch(Throwable th)  {
				Log.stack(LOG_ID,"garmin",th);
				}

		});
		var shortcuts = getbutton(context, R.string.shutcuts);
		shortcuts.setOnClickListener(v -> {
   //         context.themeLightBars();
			hidekeyboard(context);
			new Shortcuts().mkshortlistview(context);
		});
		var blackmode = getcheckbox(context,R.string.darkmode, alldata.getKerfstokBlack(context,peerId));
		blackmode.setOnCheckedChangeListener((buttonView, isChecked) ->
			alldata.setcolor(context,peerId,isChecked));
        var alarms = getbutton(context,R.string.alarms);
        alarms.setVisibility(alldata.canConfigureGarminAlarms(peerId)?VISIBLE:GONE);
		var Help = getbutton(context, R.string.helpname);
		Help.setOnClickListener(v-> help(tk.glucodata.R.string.garminconfig,context));
		var Close = getbutton(context, R.string.closename);
		var layout = new Layout(context, (l, w, h) -> {
			return new int[]{w, h};
		},new View[]{apppresent,setid}, new View[]{shortcuts, blackmode}, new View[]{alarms, Help, Close});

		float density = GlucoseCurve.metrics.density;
		int laypad = (int) (density * 4.0);
		layout.setPadding(laypad * 2, laypad * 2, laypad * 2, laypad);
		setid.setOnClickListener(v -> setidview(context, alldata,layout,parentlayout));
        alarms.setOnClickListener(v -> GarminAlarms.show(context,alldata,layout,peerId));

//		layout.setBackgroundColor(Applic.backgroundcolor);
   layout.setBackgroundResource(R.drawable.dialogbackground);

    var  params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT,  Gravity.CENTER|Gravity.CENTER_HORIZONTAL);

    context.addMyContentView(layout, params);
	//	context.addMyContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
		context.setonback(() -> {
			EnableControls(parent,true);
            closed=false;
            refreshStatus.run();
			removeContentView(layout);
			context.hideSystemUI();
			hidekeyboard(context);
		});
		Close.setOnClickListener(
				v -> context.doonback());
	}
}
