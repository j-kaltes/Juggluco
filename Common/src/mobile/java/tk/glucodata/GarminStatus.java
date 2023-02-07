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

import android.content.Intent;
import android.net.Uri;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Spanned;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.garmin.android.connectiq.IQDevice;

import java.text.DateFormat;
import java.util.Date;

import tk.glucodata.nums.AllData;
import tk.glucodata.nums.numio;
import tk.glucodata.settings.Shortcuts;

import static android.text.InputType.TYPE_NULL;
import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Natives.getkerfstokblack;
import static tk.glucodata.Natives.setkerfstokblack;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.help;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getcheckbox;
import static tk.glucodata.util.getlabel;
import static tk.glucodata.util.timestring;

//import static tk.glucodata.GlucoseCurve.width;
//import static tk.glucodata.GlucoseCurve.height;

class GarminStatus {
	Spinner spinner;
	//s/^[ 	]*\([^=]*\)=new.*/TextView \1;/g
	TextView sdkreadyview;
	TextView registeredview;
	TextView restview;
	//	TextView GarminStatusstr;
	CheckBox glucose;
	AllData alldata;
	Button next;
	Button sync;
	Layout layout;
	private static final String LOG_ID = "GarminStatus";

	static String displaystr(IQDevice device) {
		IQDevice.IQDeviceStatus stat = device.getStatus();
		String friendly = device.getFriendlyName();
		return ((friendly == null) ? device.getDeviceIdentifier() : friendly) + " - " + stat.name();
	}

	static private void setidview(MainActivity context, AllData alldata,View parent) {
		EnableControls(parent,false);
		var idlabel = getlabel(context, "Garmin Watch app ID:");

		var defaultapp = new CheckBox(context);
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
			var width = GlucoseCurve.getwidth();
			if (width > w)
				l.setX((width - w) / 2);
			l.setY(0);
			return new int[]{w, h};
		}, new View[]{idlabel, defaultapp}, new View[]{editid}, new View[]{Cancel, Save});

		float density = GlucoseCurve.metrics.density;
		int laypad = (int) (density * 4.0);
		layout.setPadding(laypad * 2, laypad * 2, laypad * 2, laypad);

		layout.setBackgroundColor(Applic.backgroundcolor);
		context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
		context.setonback(() -> {
			EnableControls(parent,true);
			removeContentView(layout);
			context.hideSystemUI();
			hidekeyboard(context);

		});
		Cancel.setOnClickListener(
				v -> context.doonback());
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
							Toast.makeText(context, "ID should be 32 characters long (now " + idlen + ")", Toast.LENGTH_SHORT).show();
							return;
						}
						if (!garminid.equals(id)) {
							if (!(changed = Natives.setgarminid(id))) {
								Toast.makeText(context, "Changed id failed", Toast.LENGTH_SHORT).show();
								return;
							}

						}
					}
					if (changed)
						alldata.reinit(context);
					context.doonback();
					context.doonback();
					context.doonback();
					new GarminStatus(context, alldata);

				});

	}


	public GarminStatus(MainActivity context, AllData alldata) {
		this.alldata = alldata;
		spinner = new Spinner(context);
		spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
			@Override
			public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
				Log.i(LOG_ID, "onItemSelected");
				if (alldata.devices != null) {
					alldata.devused = position;
					long oldident = numio.getident();
					long ident = alldata.devices.get(position).getDeviceIdentifier();
					if (oldident != ident) {
						numio.setident(ident);
						alldata.reinit(context);
					}
				}
			}

			@Override
			public void onNothingSelected(AdapterView<?> parent) {
				Toast.makeText(parent.getContext(), "Nothing selected", Toast.LENGTH_SHORT).show();
			}
		});
		RangeAdapter<IQDevice> adap = new RangeAdapter<>(alldata.devices, context, GarminStatus::displaystr);
		spinner.setAdapter(adap);
		avoidSpinnerDropdownFocus(spinner);

		sdkreadyview = new TextView(context);
		float density = GlucoseCurve.metrics.density;
		int pad = (int) density;
		int morepad = (int) (density * 4.0);
		sdkreadyview.setPadding(pad, morepad, morepad, pad * 2);
//		sdkreadyview.setPadding(0,0,0,0);
		registeredview = new TextView(context);
		restview = new TextView(context);
		sync = new Button(context);
		sync.setText(R.string.sync);
		sync.setOnClickListener(
				v -> alldata.sync());
		Button refresh = new Button(context);
		refresh.setText(R.string.refresh);
		refresh.setOnClickListener(
				v -> show());
		Button ok = new Button(context);
		ok.setText(R.string.closename);
		Button help = getbutton(context, R.string.helpname);
		help.setOnClickListener(v -> help(R.string.watchhelp, context));
		glucose = new CheckBox(context);
		glucose.setText(R.string.glucose);
		glucose.setChecked(alldata.sendtowatch);
		glucose.setOnClickListener(
				v -> {
					if (glucose.isChecked())
						alldata.startglucose();

					else
						alldata.stopglucose();
				});
		next = new Button(context);
		next.setText(R.string.sendqueue);
		next.setOnClickListener(v -> alldata.nextmessage());
		Button reinit = new Button(context);
		reinit.setText(R.string.reinit);
		reinit.setOnClickListener(v -> alldata.reinit(context));
//		var setid = getbutton(context, "ID");
//		setid.setOnClickListener(v -> setidview(context, alldata));
		Button config = getbutton(context, "Config");
		config.setOnClickListener(v -> kerfstokconfig(context,alldata,layout));
//		restview.setPadding(0,0,0,0);
		layout = new Layout(context, (l, w, h) -> {
			var width = GlucoseCurve.getwidth();
			var height = GlucoseCurve.getheight();
			if (width > w && height > h) {
				l.setX((width - w) / 2);
				l.setY((height - h) / 2);
			}
			return new int[]{w, h};
		}, new View[]{spinner, refresh}, new View[]{sdkreadyview, registeredview,help}, new View[]{restview,glucose}, new View[]{sync, next, reinit, config, ok}
		);
		layout.setBackgroundColor(Applic.backgroundcolor);
		int laypad = (int) (density * 4.0);
		layout.setPadding(laypad * 2, laypad * 2, laypad * 2, laypad*2);
		context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));

		ok.setOnClickListener(
				v -> context.doonback());
		context.setonback(() -> removeContentView(layout));
		show();
	}

	public void show() {
		try {
			if (alldata.devices != null) {
				RangeAdapter<IQDevice> adap = new RangeAdapter<IQDevice>(alldata.devices, spinner.getContext(), GarminStatus::displaystr);
				spinner.setAdapter(adap);
				spinner.setSelection(alldata.devused);
				//		spinner.getAdapter().notifyDataSetChanged();
			}
			sdkreadyview.setText("SDK ready: " + (alldata.sdkready() ? "Yes" : "No"));
			registeredview.setText("Registered: " + (alldata.usewatch ? "Yes" : "No"));
	/*	if(alldata.appmissing==0)
			apppresent.setVisibility(INVISIBLE);
		else*/
			StringBuilder builder = new StringBuilder();
			if (alldata.sendtime != 0L) {
				builder.append("Send: Yes\t\t");
				builder.append(timestring(alldata.sendtime));
				builder.append("\n");
				if (alldata.sendtime > alldata.statustime) {
					builder.append("No status information\n");
				} else {
					builder.append("Status:\t");
					builder.append(alldata.sendstatus.name());
					builder.append("\n");
				}
			} else {
				builder.append("Send: No\n");
			}
			if (alldata.receivedmessage == 0) {
				builder.append("Received: No");
			} else {
				builder.append("Received: Yes\t\t");
				builder.append(timestring(alldata.receivedmessage));
			}
			String alltext=builder.toString();
			Log.i(LOG_ID,"setText "+alltext);
			restview.setText(alltext);
			int vis = alldata.usewatch ? VISIBLE : GONE;
			glucose.setChecked(alldata.sendtowatch);
			glucose.setVisibility(vis);
			next.setVisibility(alldata.usewatch && alldata.waiting() ? VISIBLE : GONE);
			sync.setVisibility(vis);
//		resend.setVisibility(vis);
			layout.setVisibility(VISIBLE);
//			layout.invalidate();
		} catch (Throwable e) {
			String mess = e.getMessage();
			if (mess == null)
				mess = "GarminStatus error";
			Log.e(LOG_ID, "Exception: " + mess);
		}

	}

	static private void kerfstokconfig(MainActivity context,AllData alldata,View parent) {
		EnableControls(parent,false);
		var setid = getbutton(context, "ID");

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
			hidekeyboard(context);
			new Shortcuts().mkshortlistview(context);
		});
		var blackmode = getcheckbox(context,R.string.darkmode, getkerfstokblack());
		blackmode.setOnCheckedChangeListener((buttonView, isChecked) -> {
			Applic.app.numdata.setcolor(isChecked);
			setkerfstokblack(isChecked);
		});
		var Help = getbutton(context, R.string.helpname);
		Help.setOnClickListener(v-> help.help(tk.glucodata.R.string.garminconfig,context));
		var Close = getbutton(context, R.string.closename);
		var layout = new Layout(context, (l, w, h) -> {
			var width = GlucoseCurve.getwidth();
			var height = GlucoseCurve.getheight();
			if (width > w)
				l.setX((width - w) / 2);
			if (height > h)
				l.setY((height - h) / 2);
			return new int[]{w, h};
		},new View[]{apppresent,setid}, new View[]{shortcuts, blackmode}, new View[]{Help, Close});

		float density = GlucoseCurve.metrics.density;
		int laypad = (int) (density * 4.0);
		layout.setPadding(laypad * 2, laypad * 2, laypad * 2, laypad);
		setid.setOnClickListener(v -> setidview(context, alldata,layout));
		layout.setBackgroundColor(Applic.backgroundcolor);
		context.addContentView(layout, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
		context.setonback(() -> {
			EnableControls(parent,true);
			removeContentView(layout);
			context.hideSystemUI();
			hidekeyboard(context);
		});
		Close.setOnClickListener(
				v -> context.doonback());
	}
}
