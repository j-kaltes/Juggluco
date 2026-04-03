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

public final class UnpairOverlayHost {
    private final Activity activity;

    private View overlay;
    private TextView titleView;
    private TextView messageView;
    private ProgressBar spinner;
    private Button closeButton;
    private boolean destroyed;
 static private final String LOG_ID="UnpairOverlayHost";
    public UnpairOverlayHost(Activity activity,String title) {
        Log.i(LOG_ID,LOG_ID+"(act,"+title+")");
        this.activity = activity;

        activity.runOnUiThread(()-> ensureAdded(title));
    }
    public UnpairOverlayHost(Activity activity,int res) {
          this(activity,activity.getString(res)); 
        }

    public void destroy() {
        destroyed = true;
        Applic.getHandler().post(new Runnable() {
            @Override
            public void run() {
                removeNow();
            }
        });
    }

    public void postMessage(final String message) {
        Applic.getHandler().post(new Runnable() {
            @Override
            public void run() {
                if (destroyed || activity.isFinishing()) {
                    return;
                }


                messageView.setText(message);
                spinner.setVisibility(View.VISIBLE);
                closeButton.setVisibility(View.GONE);
                overlay.setVisibility(View.VISIBLE);
            }
        });
    }

    public void postFinished(final String message, final boolean success) {
        Applic.getHandler().post(new Runnable() {
            @Override
            public void run() {
                if (destroyed || activity.isFinishing()) {
                    return;
                }


                titleView.setText("");
                messageView.setText(message);
                overlay.setVisibility(View.VISIBLE);
            }
        });
    }
    public void postCloser() {
        Applic.getHandler().post(new Runnable() {
            @Override
            public void run() {
                if (destroyed || activity.isFinishing()) {
                    return;
                }

                spinner.setVisibility(View.GONE);
                MainActivity.setonback(() -> {
                    destroyed = true;
                    removeNow();
                    });
                closeButton.setVisibility(View.VISIBLE);
                overlay.setVisibility(View.VISIBLE);
            }
        });
    }

    public void postHide() {
        Applic.getHandler().post(new Runnable() {
            @Override
            public void run() {
                if (overlay != null) {
                    overlay.setVisibility(View.GONE);
                }
            }
        });
    }

    private void ensureAdded(final String title) {

        LayoutInflater inflater = LayoutInflater.from(activity);
        overlay = inflater.inflate(R.layout.view_unpair_progress_overlay, null, false);

        titleView = overlay.findViewById(R.id.unpairTitle);
        titleView.setText(title);
        messageView = overlay.findViewById(R.id.unpairMessage);
        spinner = overlay.findViewById(R.id.unpairSpinner);
        closeButton = overlay.findViewById(R.id.unpairCloseButton);

        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                MainActivity.doonback();
              }
        });

        activity.addContentView(
                overlay,
                new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                )
        );

        overlay.setVisibility(View.GONE);
    }

    private void removeNow() {
        if(overlay == null) {
            return;
           }
        removeContentView(overlay);
        overlay = null;
        titleView = null;
        messageView = null;
        spinner = null;
        closeButton = null;
       }
}
