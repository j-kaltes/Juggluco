package tk.glucodata;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;

public final class Confirm {
static public void	ask(Context context, String title, String mess, Runnable okproc) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setTitle(title).
	 setMessage(mess).
           setPositiveButton(R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
		    	okproc.run();
                    }
                }) .setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }
        }).show();
	}
}
