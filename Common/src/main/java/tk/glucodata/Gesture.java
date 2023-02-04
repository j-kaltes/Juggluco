
package tk.glucodata;

import static tk.glucodata.Notify.removefloating;
import static tk.glucodata.Notify.rewritefloating;

import android.graphics.PointF;
import android.view.MotionEvent;
import android.view.View;

public class Gesture implements View.OnTouchListener {
	private static String LOG_ID="Gesture";
	private   boolean moved =false;
	private long downstart;
	private int mindowntime=500;
	private float startX,startY;

    public boolean onTouch(View view, MotionEvent event) {
        Log.i(LOG_ID,event.toString());
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_DOWN:
                Log.i(LOG_ID,"Down");
		startX= event.getX();
		startY= event.getY();
                moved =false;
		downstart=event.getEventTime();
                break;
            case MotionEvent.ACTION_MOVE: {
		var distanceX= event.getX() - startX;
		var distanceY= event.getY() - startY;
	        Log.i(LOG_ID,"DRAG dx="+distanceX+" dy="+distanceY);
		if(distanceX!=0.0||distanceY!=0.0)  {
		    moved =true;
		    Notify.translate(distanceX*.45f,distanceY*.45f);
		    }
	  	else {
			if(!moved) {
				if((event.getEventTime()-downstart)>mindowntime)
				    removefloating();
				    }
			}
			

                }
                break;
        }
        return true; 
    }

}
