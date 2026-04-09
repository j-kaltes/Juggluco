package tk.glucodata;

import static android.view.View.LAYOUT_DIRECTION_LOCALE;
import static android.view.View.TEXT_ALIGNMENT_VIEW_START;
import static android.view.View.TEXT_DIRECTION_LOCALE;
import static androidx.core.graphics.drawable.DrawableCompat.setLayoutDirection;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.widget.CheckBox;
import android.widget.RadioButton;

import androidx.core.text.TextUtilsCompat;

import java.util.Locale;

public class CheckDirectionRadio extends RadioButton {
    
    public CheckDirectionRadio(Context context) {
        this(context, null);
    }

    public CheckDirectionRadio(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.radioButtonStyle);
    }

    public CheckDirectionRadio(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
    /*
        Locale locale = getResources().getConfiguration().getLocales().get(0);
        boolean rtl = TextUtilsCompat.getLayoutDirectionFromLocale(locale) == View.LAYOUT_DIRECTION_RTL;
        */

        if(MainActivity.rtl&&!Applic.supportsRtl) {
            // 1. Capture the radio circle drawable
            Drawable buttonDrawable = getButtonDrawable();
            
            // 2. Remove the locked-to-left default button
            setButtonDrawable(null);
            
            // 3. Place it on the right
            setCompoundDrawablesWithIntrinsicBounds(null, null, buttonDrawable, null);
            
            // 4. Adjust alignment
            setGravity(Gravity.RIGHT | Gravity.CENTER_VERTICAL);
            
            // 5. Optional: Add spacing between text and the radio circle
            // (Adjust 20 to whatever pixel value looks best in your layout)
//            setCompoundDrawablePadding(20); 
        } else {
        //    setGravity(Gravity.LEFT | Gravity.CENTER_VERTICAL);
        }
    }
}
