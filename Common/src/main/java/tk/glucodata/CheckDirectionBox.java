package tk.glucodata;

import static android.view.View.LAYOUT_DIRECTION_LOCALE;
import static android.view.View.TEXT_ALIGNMENT_VIEW_START;
import static android.view.View.TEXT_DIRECTION_LOCALE;
import static androidx.core.graphics.drawable.DrawableCompat.setLayoutDirection;

import static tk.glucodata.GlucoseCurve.dpToPx;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.widget.CheckBox;

import androidx.core.text.TextUtilsCompat;

import java.util.Locale;


public class CheckDirectionBox extends androidx.appcompat.widget.AppCompatCheckBox {

    public CheckDirectionBox(Context context) {
        super(context);
        init(null);
    }

    public CheckDirectionBox(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs);
    }

    public CheckDirectionBox(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(attrs);
    }

    private void init(AttributeSet attrs) {
        // Grab the drawable that was already set by the XML or the constructor
        applyDirectionalDrawable(getButtonDrawable());
    }

    @Override
    public void setButtonDrawable(Drawable drawable) {
        if(MainActivity.rtl&&!Applic.supportsRtl) {
                applyDirectionalDrawable(drawable);
                }
        else {
            super.setButtonDrawable(drawable);
        }
    }

    @Override
    public void setButtonDrawable(int resId) {
        if(MainActivity.rtl&&!Applic.supportsRtl) {
            setButtonDrawable(getContext().getDrawable(resId));
            }
        else
            super.setButtonDrawable(resId);
    }

    private void applyDirectionalDrawable(Drawable drawable) {
        if (drawable == null) return;
/*
        Locale locale = getResources().getConfiguration().getLocales().get(0);
        boolean rtl = TextUtilsCompat.getLayoutDirectionFromLocale(locale) == View.LAYOUT_DIRECTION_RTL;
        */

        if (MainActivity.rtl&&!Applic.supportsRtl) {
            // Remove from the "standard" left slot
            super.setButtonDrawable(null);

            // Fix: Some Android versions require the drawable to be told to update its visual state
            drawable.setState(getDrawableState());

            // Place it in the right compound slot
            setCompoundDrawablesWithIntrinsicBounds(null, null, drawable, null);
            setGravity(Gravity.RIGHT | Gravity.CENTER_VERTICAL);
        } else {
        /*
            // Restore standard LTR
            setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
            super.setButtonDrawable(drawable);
            setGravity(Gravity.LEFT | Gravity.CENTER_VERTICAL);
            */
        }
    }

    // Crucial: This ensures the checkbox square actually changes color/state when clicked
    @Override
    protected void drawableStateChanged() {
        super.drawableStateChanged();
        if(MainActivity.rtl&&!Applic.supportsRtl) {
            Drawable[] drawables = getCompoundDrawables();
            if (drawables[2] != null) { // Index 2 is the Right Drawable
                drawables[2].setState(getDrawableState());
                invalidate();
            }
            }
    }
}
