package tk.glucodata;

import static tk.glucodata.Notify.penmutable;
import static tk.glucodata.Notify.timef;

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.appwidget.AppWidgetProviderInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.widget.RemoteViews;

public class GlucoseWidget extends AppWidgetProvider {
    static RemoteGlucose remote = null;
     static int width=0;

private static void setWidth(int widthdip) {
		var    widthpx = widthdip*GlucoseCurve.getDensity();
//		var fontsize=widthpx*0.22f;
		var fontsize=widthpx*(Applic.unit==1?0.35f:0.4f);
		remote=  new RemoteGlucose(fontsize,widthpx,Applic.unit==1?0.30f:0.32f,2,true);
		width=widthdip;
		}
 @Override
    public void onAppWidgetOptionsChanged (Context context, AppWidgetManager appWidgetManager, int appWidgetId, Bundle widgetInfo) {
	var widthdip=widgetInfo.getInt(AppWidgetManager.OPTION_APPWIDGET_MIN_WIDTH);
       Log.i(LOG_ID,"onAppWidgetOptionsChanged widthdip="+widthdip);
       used=true;
	if(widthdip!=0&&widthdip!=width) {
		setWidth(widthdip);
            	updateAppWidget(context, appWidgetManager, appWidgetId);
		}
       } 
static private RemoteViews remoteMessage(String message) {
	  RemoteViews remoteViews = new RemoteViews(Applic.app.getPackageName(), R.layout.text);
                remoteViews.setTextColor(R.id.content, Notify.foregroundcolor);
                remoteViews.setTextViewText(R.id.content, message);
		return remoteViews;
	}
static private long oldage=60*3*1000L;

static private void showviews(RemoteViews views,int rId,AppWidgetManager appWidgetManager, int appWidgetId) {
	 Intent intent = new Intent(Applic.app, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(Applic.app, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT|penmutable);
        views.setOnClickPendingIntent(rId, pendingIntent);
        appWidgetManager.updateAppWidget(appWidgetId, views);
	}
    static private void updateAppWidget(Context context, AppWidgetManager appWidgetManager, int appWidgetId) {
	var widgetInfo=appWidgetManager.getAppWidgetOptions(appWidgetId);
	for(var key : widgetInfo.keySet()) {
	     Log.d(LOG_ID, key + " = " + widgetInfo.get(key));
	     }
	var widthdip=widgetInfo.getInt(AppWidgetManager.OPTION_APPWIDGET_MIN_WIDTH);
	if(widthdip==0) {
		if(remote==null)
			widthdip=200;
		else
			widthdip=width;
		} if(widthdip!=width) {
		setWidth(widthdip);
		}
	RemoteViews  views;
	strGlucose glu;
	int id= R.id.arrowandvalue;
        if(SuperGattCallback.previousglucose != null|| (((glu=Natives.lastglucose())!=null) && ((SuperGattCallback.previousglucose = new notGlucose(glu.time * 1000L, glu.value, glu.rate))!=null))) {

		final var now=System.currentTimeMillis();
//		SuperGattCallback.previousglucose.value="27.8";
	//	SuperGattCallback.previousglucose.value="388";
		final var time=SuperGattCallback.previousglucose.time;
		if((now-time)>oldage) {
			final String tformat= timef.format(time);
			String message = "\n  "+Applic.app.getString(R.string.nonewvalue) + tformat;
			views=remoteMessage(message);
			id=R.id.content;
		}
		else {
			views = remote.arrowremote(50,SuperGattCallback.previousglucose);
			}
		}
	else {
			views=remoteMessage("\n  "+Applic.app.getString(R.string.novalue));
			id=R.id.content;
			}

	showviews(views,id,  appWidgetManager, appWidgetId);
    }

static private void updateall(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {
	if(appWidgetIds==null|| appWidgetIds.length == 0) {
		Log.i(LOG_ID,"updateall zero");
		return;
		}
	used=true;
        for (int appWidgetId : appWidgetIds) {
            updateAppWidget(context, appWidgetManager, appWidgetId);
        }
	}

static boolean used=true;
final    private static String LOG_ID="GlucoseWidget";
    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds) {
        Log.i(LOG_ID,"onUpdate");
    	updateall(context, appWidgetManager,  appWidgetIds);
    }

    @Override
    public void onEnabled(Context context) {
	used=true;
        // Enter relevant functionality for when the first widget is created
    }

    @Override
    public void onDisabled(Context context) {
        // Enter relevant functionality for when the last widget is disabled
	used=false;
    }
public static void oldvalue(long time) {
	final var cl= GlucoseWidget.class;
	final var manage= AppWidgetManager.getInstance(Applic.app);
	int ids[] = manage.getAppWidgetIds(new ComponentName(Applic.app, cl));
	if(ids.length>0) {
		Log.i(LOG_ID,"oldvalue widgets");
		final String tformat= timef.format(time);
		String message = Applic.app.getString(R.string.nonewvalue) + tformat;
		var views=remoteMessage(message);
		for(var id:ids) {
			showviews(views,R.id.content,manage,id);
			}
		}
	else {
		Log.i(LOG_ID,"oldvalue no widgets");
		}

	}
 public static void update() {
 	if(used) {
		final var cl= GlucoseWidget.class;
		final var manage= AppWidgetManager.getInstance(Applic.app);
		int ids[] = manage.getAppWidgetIds(new ComponentName(Applic.app, cl));
		if(ids.length>0) {
			updateall(Applic.app, manage, ids);
			}
		else
			used=false;
		}
    }

}
