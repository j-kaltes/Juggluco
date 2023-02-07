
package tk.glucodata;

import static tk.glucodata.Floating.removefloating;
import static tk.glucodata.Floating.rewritefloating;

import android.graphics.PointF;
import android.view.MotionEvent;
import android.view.View;

public class BackGesture implements View.OnTouchListener {
	private static final String LOG_ID="BackGesture";
private final MainActivity main;
private final float 	miny,maxy,minx,maxx;
BackGesture(MainActivity act) {
	int height=GlucoseCurve.getheight();
	miny=height*.1f;
	maxy=height*.9f;
	int width=GlucoseCurve.getwidth();
	minx=width*.1f;
	maxx=width*.9f;
	main=act;
	}
private boolean backed=false;
    public boolean onTouch(View view, MotionEvent event) {
        Log.i(LOG_ID,event.toString());
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
		case MotionEvent.ACTION_BUTTON_PRESS:
            case MotionEvent.ACTION_DOWN:
	    	if(!backed) {
			final var x=event.getX();
			final var y=event.getY();
			if(x<minx||x>maxx||y<miny||y>maxy) {
				backed=true;
				main.doonback();
				}
			}
                break;

        	}
        return true; 
    }

}
