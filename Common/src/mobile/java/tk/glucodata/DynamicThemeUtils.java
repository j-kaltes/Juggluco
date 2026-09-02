package tk.glucodata;

import androidx.core.content.ContextCompat;
import android.app.Activity;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.InsetDrawable;
import android.graphics.drawable.RippleDrawable;
import android.graphics.drawable.StateListDrawable;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.TextView;

import tk.glucodata.settings.AppTheme;

public class DynamicThemeUtils {
private static final String LOG_ID="DynamicThemeUtils";

    public static int resolveAttributeColor(Context ctx, int attrId, int defaultColor) {
        if (attrId == 0) return defaultColor;

        TypedValue val = new TypedValue();

        if (ctx.getTheme().resolveAttribute(attrId, val, true)) {
            try {
                if (val.resourceId != 0) {
                    return ContextCompat.getColor(ctx, val.resourceId);
                }

                if (val.type >= TypedValue.TYPE_FIRST_COLOR_INT &&
                        val.type <= TypedValue.TYPE_LAST_COLOR_INT) {
                    return val.data;
                }
            } catch (Throwable e) {
                Log.stack(LOG_ID, "resolveAttributeColor", e);
            }
        }
        return defaultColor;
    }
    public static InsetDrawable createDynamicButton(Context ctx, int radiusDp, boolean isOval) {
        String pkg = ctx.getPackageName();
        
        int bgId = ctx.getResources().getIdentifier("colorButtonBackground", "attr", pkg);
        int disabledBgId = ctx.getResources().getIdentifier("colorButtonDisabledBackground", "attr", pkg);
        int highId = ctx.getResources().getIdentifier("colorControlHighlight", "attr", pkg);

        int nativeBtnColor = resolveAttributeColor(ctx, android.R.attr.colorButtonNormal, Color.LTGRAY);
        
        int cBg = resolveAttributeColor(ctx, bgId, nativeBtnColor);
        int cDisabledBg = resolveAttributeColor(ctx, disabledBgId, Color.GRAY);
        int cHigh = resolveAttributeColor(ctx, highId, 0x33FFFFFF);

        float density = ctx.getResources().getDisplayMetrics().density;
        int radiusPx = (int) (radiusDp * density);
        int shape = isOval ? GradientDrawable.OVAL : GradientDrawable.RECTANGLE;

        GradientDrawable normShape = new GradientDrawable();
        normShape.setShape(shape);
        normShape.setColor(cBg);
        if (!isOval) normShape.setCornerRadius(radiusPx);

        GradientDrawable disShape = new GradientDrawable();
        disShape.setShape(shape);
        disShape.setColor(cDisabledBg);
        if (!isOval) disShape.setCornerRadius(radiusPx);

        StateListDrawable stateList = new StateListDrawable();
        stateList.addState(new int[]{-android.R.attr.state_enabled}, disShape);
        stateList.addState(new int[]{}, normShape);

        GradientDrawable mask = new GradientDrawable();
        mask.setShape(shape);
        mask.setColor(Color.WHITE);
        if (!isOval) mask.setCornerRadius(radiusPx);

        RippleDrawable ripple = new RippleDrawable(ColorStateList.valueOf(cHigh), stateList, mask);

        int insetH = (int) (4 * density);
        int insetV = (int) (6 * density);
        
        return new InsetDrawable(ripple, insetH, insetV, insetH, insetV);
    }

    public static void applyTheme(View root) {
        if(Natives.getTheme() == 0) {
            return;
           }
        int radius = Natives.getradius();
        boolean isOval = Natives.getisOval();
        traverseAndStyle(root, radius, isOval, true);
       }

    public static void applyTheme(View root,int radius,boolean isOval) {
        traverseAndStyle(root, radius, isOval, true);
       }

    private static void applyDisabledTextColor(TextView text) {
        ColorStateList oldColors = text.getTextColors();
//        if(oldColors == null || oldColors.isStateful())
        if(oldColors == null)
            return;

        Context ctx = text.getContext();
        String pkg = ctx.getPackageName();
        int disabledId = ctx.getResources().getIdentifier("colorDisabledText", "attr", pkg);
        int enabledColor = oldColors.getDefaultColor();
        int disabledColor = resolveAttributeColor(ctx, disabledId, enabledColor);

        text.setTextColor(new ColorStateList(
                new int[][] {
                        new int[] {-android.R.attr.state_enabled},
                        new int[] {}
                },
                new int[] {disabledColor, enabledColor}
        ));
    }

    private static void applyCompoundButtonTint(CompoundButton button) {
        Context ctx = button.getContext();
        String pkg = ctx.getPackageName();
        int disabledId = ctx.getResources().getIdentifier("colorDisabledControl", "attr", pkg);

        int normal = resolveAttributeColor(ctx, android.R.attr.colorControlNormal, Color.GRAY);
        int activated = resolveAttributeColor(ctx, android.R.attr.colorControlActivated, normal);
        int disabled = resolveAttributeColor(ctx, disabledId, normal);

        button.setButtonTintList(new ColorStateList(
                new int[][] {
                        new int[] {-android.R.attr.state_enabled},
                        new int[] {android.R.attr.state_checked},
                        new int[] {}
                },
                new int[] {disabled, activated, normal}
        ));
    }

    private static void traverseAndStyle(View view, int radius, boolean isOval, boolean styleButtons) {
        if (view instanceof TextView)
            applyDisabledTextColor((TextView)view);

        if (view instanceof CompoundButton)
            applyCompoundButtonTint((CompoundButton)view);

        if (styleButtons && view instanceof Button && !(view instanceof CompoundButton)) {
            int pL = view.getPaddingLeft();
            int pT = view.getPaddingTop();
            int pR = view.getPaddingRight();
            int pB = view.getPaddingBottom();

            view.setBackground(createDynamicButton(view.getContext(), radius, isOval));

            view.setPadding(pL, pT, pR, pB);
        }

        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) {
                traverseAndStyle(group.getChildAt(i), radius, isOval, styleButtons);
            }
        }
    }
static void setTheme(Activity act) {
       int theme=Natives.getTheme();
       Log.i(LOG_ID,"getTheme()="+theme);
       if(theme!=0) {
                act.setTheme( AppTheme.values()[theme].getStyleResId());
                if(Natives.getisOval()) {
                    act.getTheme().applyStyle(R.style.ShapeOverride_Oval, true);
                    }
             }
       }
}
