package tk.glucodata;

import static tk.glucodata.settings.Settings.removeContentView;

import android.app.Activity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import tk.glucodata.Log;

public final class UnpairOverlayHost {
 static private final String LOG_ID="UnpairOverlayHost";

    public UnpairOverlayHost(Object activity,String title) {
        Log.i(LOG_ID,LOG_ID+"(act,"+title+")");
        }
    public UnpairOverlayHost(Object activity,int res) {
        Log.i(LOG_ID,"UnpairOverlayHost act "+res);
        }
    public void postFinished(final String message, final boolean success) {
        }
    public void postCloser() {
        }

}
