
package tk.glucodata;
import android.content.Intent;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;

import androidx.annotation.UiThread;

class GestureListener implements GestureDetector.OnGestureListener {
    public GestureListener() {

    }

static private final String LOG_ID="GestureListener";
        @Override
        public boolean onDown(MotionEvent e) {
	    Log.i(LOG_ID,"onDown");
            return true;
        }

        @Override
        public void onShowPress(MotionEvent e) {
	    Log.i(LOG_ID,"onShowPress");
        }


        @UiThread
        @Override
        public boolean onSingleTapUp(MotionEvent event) {
            Log.d(LOG_ID,"onSingleTapUp");
		    return true;
        	}



        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
		Log.i(LOG_ID,"Scroll e2="+e2.toString());
		    Notify.translate(distanceX,distanceY);
        	return true;
        	}


        @Override
        public void onLongPress(MotionEvent event) {
            Log.d(LOG_ID,"OnLongPress" );
            }
/*
        @Override
        public boolean	onDoubleTap(MotionEvent e) {
            Log.d(LOG_ID,"public boolean	onDoubleTap(MotionEvent e) {");return false;

        }

        @Override
        public boolean	onDoubleTapEvent(MotionEvent e) {
            Log.d(LOG_ID,"public boolean	onDoubleTapEvent(MotionEvent e) {");return false;

        }

        @Override
        public   boolean	onSingleTapConfirmed(MotionEvent e) {
            Log.d(LOG_ID,"public   boolean	onSingleTapConfirmed(MotionEvent e) {");return false;
        } */
        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
		Log.d(LOG_ID,"onFling");
                return true;
        }

    };//end class GestureListener 

