package tk.glucodata.settings;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

import static tk.glucodata.DynamicThemeUtils.applyTheme;
import static tk.glucodata.Layout.getMargins;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.okbutton;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.os.Build;
import android.util.TypedValue;
import android.view.ContextThemeWrapper;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;

import tk.glucodata.Backup;
import tk.glucodata.Consumer;
import tk.glucodata.DynamicThemeUtils;
import tk.glucodata.GlucoseCurve;
import tk.glucodata.Layout;
import tk.glucodata.Log;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;
import tk.glucodata.R;
import tk.glucodata.help;

public class SelectTheme {

private static final String LOG_ID="SelectTheme";
    private static AppTheme selectedTheme;
    private static int currentRadius = 12;
    private static boolean currentIsOval = false;


private static class ReselectSpinner extends Spinner {
    private AdapterView.OnItemSelectedListener listener;

    public ReselectSpinner(Context context) {
        super(context);
    }

    public ReselectSpinner(Context context, int mode) {
        super(context, mode);
    }

    @Override
    public void setOnItemSelectedListener(AdapterView.OnItemSelectedListener listener) {
        this.listener = listener;
        super.setOnItemSelectedListener(listener);
    }

    @Override
    public void setSelection(int position) {
        boolean sameSelection = position == getSelectedItemPosition();
        super.setSelection(position);

        if(sameSelection && listener != null) {
            listener.onItemSelected( this, getSelectedView(), position, getSelectedItemId());
           }
    }

    @Override
    public void setSelection(int position, boolean animate) {
        boolean sameSelection = position == getSelectedItemPosition();

        super.setSelection(position, animate);

        if (sameSelection && listener != null) {
            listener.onItemSelected(
                    this,
                    getSelectedView(),
                    position,
                    getSelectedItemId()
            );
        }
    }
}
    /**
     * Closed spinner view shows only "Light" or "Dark".
     * Drop-down rows show the actual theme names.
     */

    private static BaseAdapter createThemePickerAdapter(Context context, AppTheme[] items, String closedText) {
        return new BaseAdapter() {
            @Override
            public int getCount() {
                return items.length;
            }

            @Override
            public AppTheme getItem(int position) {
                return items[position];
            }

            @Override
            public long getItemId(int position) {
                return position;
            }

            @Override
            public boolean areAllItemsEnabled() {
                return true;
            }

            @Override
            public boolean isEnabled(int position) {
                return true;
            }

            @NonNull
            @Override
            public View getView(int position, View convertView, @NonNull ViewGroup parent) {
                TextView tv = convertView instanceof TextView
                        ? (TextView) convertView
                        : createSpinnerTextView(context, false);
                tv.setText(closedText);

                // The closed spinner must match the preview panel, not the real app theme.
                styleSpinnerText(context, tv, selectedTheme);
                return tv;
            }

            @NonNull
            @Override
            public View getDropDownView(int position, View convertView, @NonNull ViewGroup parent) {
                TextView tv = convertView instanceof TextView
                        ? (TextView) convertView
                        : createSpinnerTextView(context, true);
                tv.setText(items[position].toString());

                // Each dropdown row is styled with its own theme. This avoids AndroidDefault
                // platform spinner/list styling making unselected light-theme rows invisible.
                styleSpinnerText(context, tv, items[position]);
                return tv;
            }
        };
    }

    private static TextView createSpinnerTextView(Context context, boolean dropDownRow) {
        TextView tv = new TextView(context);
        float density = context.getResources().getDisplayMetrics().density;

        int paddingH = (int) ((dropDownRow ? 16 : 8) * density);
        int paddingV = (int) ((dropDownRow ? 10 : 6) * density);
        tv.setPadding(paddingH, paddingV, paddingH, paddingV);
        tv.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
        tv.setSingleLine(true);
        tv.setIncludeFontPadding(true);
        tv.setEnabled(true);
        tv.setAlpha(1.0f);
        tv.setVisibility(View.VISIBLE);
        tv.setTextAlignment(View.TEXT_ALIGNMENT_GRAVITY);

        if (dropDownRow) {
            tv.setMinHeight((int) (48 * density));
            tv.setLayoutParams(new AbsListView.LayoutParams(
                    AbsListView.LayoutParams.MATCH_PARENT,
                    AbsListView.LayoutParams.WRAP_CONTENT
            ));
        } else {
            tv.setMinHeight((int) (40 * density));
            tv.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
            ));
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            tv.setForeground(null);
        }

        return tv;
    }

    private static void styleSpinnerText(Context context, TextView tv, AppTheme rowTheme) {
        Context themeContext = new ContextThemeWrapper(context, rowTheme.getStyleResId());

        int spinnerTextAttr = context.getResources().getIdentifier( "colorSpinnerDropdownText", "attr", context.getPackageName());

        int textColor = DynamicThemeUtils.resolveAttributeColor(
                themeContext,
                spinnerTextAttr,
                DynamicThemeUtils.resolveAttributeColor(
                        themeContext,
                        android.R.attr.textColorPrimary,
                        Color.BLACK
                )
        );

        int bgColor = DynamicThemeUtils.resolveAttributeColor(
                themeContext,
                android.R.attr.windowBackground,
                rowTheme.isDark() ? Color.BLACK : Color.WHITE
        );

        int highlightColor = DynamicThemeUtils.resolveAttributeColor(
                themeContext,
                android.R.attr.colorControlHighlight,
                rowTheme.isDark() ? 0x33FFFFFF : 0x33000000
        );

        StateListDrawable rowBackground = new StateListDrawable();
        rowBackground.addState(new int[]{android.R.attr.state_pressed}, new ColorDrawable(highlightColor));
        rowBackground.addState(new int[]{android.R.attr.state_focused}, new ColorDrawable(highlightColor));
        rowBackground.addState(new int[]{android.R.attr.state_selected}, new ColorDrawable(highlightColor));
        rowBackground.addState(new int[]{android.R.attr.state_checked}, new ColorDrawable(highlightColor));
        rowBackground.addState(new int[]{}, new ColorDrawable(bgColor));

        ColorStateList textColors = new ColorStateList(
                new int[][] {
                        new int[] { -android.R.attr.state_enabled },
                        new int[] { android.R.attr.state_pressed },
                        new int[] { android.R.attr.state_focused },
                        new int[] { android.R.attr.state_selected },
                        new int[] { android.R.attr.state_checked },
                        new int[] {}
                },
                new int[] {
                        textColor,
                        textColor,
                        textColor,
                        textColor,
                        textColor,
                        textColor
                }
        );

        tv.setTextColor(textColors);
        tv.setBackground(rowBackground);
        tv.setAlpha(1.0f);
        tv.setEnabled(true);
        tv.setVisibility(View.VISIBLE);
        tv.setTextAlignment(View.TEXT_ALIGNMENT_GRAVITY);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            tv.setForeground(null);
        }
    }

    private static boolean isShapeEditorAvailable() {
        return selectedTheme != AppTheme.DEFAULT;
    }

    private static void setShapeEditorVisibility(boolean visible,
                                                 TextView shapeLabel,
                                                 TextView radiusLabel,
                                                 SeekBar radiusSeekBar,
                                                 RadioButton rectButton,
                                                 RadioButton ovalButton) {
        int visibility = visible ? View.VISIBLE : View.GONE;

        shapeLabel.setVisibility(visibility);
        radiusLabel.setVisibility(visibility);
        radiusSeekBar.setVisibility(visibility);
        rectButton.setVisibility(visibility);
        ovalButton.setVisibility(visibility);

        rectButton.setEnabled(visible);
        ovalButton.setEnabled(visible);
        radiusSeekBar.setEnabled(visible && !currentIsOval);
    }

    public static View createThemeConfigurationPanel(MainActivity act,View parent) {
        Context context = act;

        currentRadius = Natives.getradius();
        currentIsOval = Natives.getisOval();
        int currentThemeIdx = Natives.getTheme();
        selectedTheme = (currentThemeIdx >= 0 && currentThemeIdx < AppTheme.values().length)
                ? AppTheme.values()[currentThemeIdx] : AppTheme.DEFAULT;

        float density = context.getResources().getDisplayMetrics().density;
        int p15 = dp(context, 15);
        int p8 = dp(context, 8);
        int p12 = dp(context, 12);

        

        TextView themeName = new TextView(context);
        themeName.setTextSize(20);
        themeName.setGravity(Gravity.CENTER_HORIZONTAL);

        TextView themesLabel = new TextView(context);
        themesLabel.setText(R.string.themesname);


        AppTheme[] lightThemes = AppTheme.getThemesByGroup(false);
        AppTheme[] darkThemes = AppTheme.getThemesByGroup(true);

        // Do not create these with the currently applied app theme. When AndroidDefault
        // is active it can give the popup/list dark native drawables before a preview
        // light theme has been selected.
        Spinner lightSpinner = new ReselectSpinner(new ContextThemeWrapper(context, AppTheme.LIGHT.getStyleResId()));
        Spinner darkSpinner = new ReselectSpinner(new ContextThemeWrapper(context, AppTheme.DARK.getStyleResId()));

        lightSpinner.setAdapter(createThemePickerAdapter(context, lightThemes, context.getString(R.string.light)));
        darkSpinner.setAdapter(createThemePickerAdapter(context, darkThemes, context.getString(R.string.dark)));
        getMargins(darkSpinner).rightMargin=p8;

        TextView radiusLabel = new TextView(context);
        radiusLabel.setText(context.getString(R.string.radius) + currentRadius + "dp");

        SeekBar radiusSeekBar = new SeekBar(context);
        radiusSeekBar.setMax(25);
        radiusSeekBar.setProgress(currentRadius);
        int width= GlucoseCurve.getwidth();
        int height= GlucoseCurve.getheight();
        int largest=Math.max(width,height);
        radiusSeekBar.setMinimumWidth((int)(largest*.25f));


        var shapelabel=getlabel(context,R.string.buttonshape);



        RadioButton rectButton = new RadioButton(context);
        rectButton.setId(View.generateViewId());
        rectButton.setText(R.string.rectangle);

        RadioButton ovalButton = new RadioButton(context);
        ovalButton.setId(View.generateViewId());
        ovalButton.setText(R.string.oval);

        getMargins(ovalButton).rightMargin=p8;


        final boolean[] internalUpdate = { false };
        final boolean[] spinnerReady = { false };

        setSpinnerSelection(lightSpinner, lightThemes, selectedTheme);
        setSpinnerSelection(darkSpinner, darkThemes, selectedTheme);
        if (currentIsOval) ovalButton.setChecked(true); else rectButton.setChecked(true);
        radiusSeekBar.setEnabled(isShapeEditorAvailable() && !currentIsOval);
        var root=new Layout(context,new View[]{themeName},new View[]{themesLabel,lightSpinner,darkSpinner},new View[]{shapelabel,rectButton,ovalButton},new View[]{radiusLabel,radiusSeekBar});

        root.setPadding(p12, p12, p12, p8);
        root.systembarMargins();

        AdapterView.OnItemSelectedListener lightListener = new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                    if (internalUpdate[0] || !spinnerReady[0]) return;
                    selectTheme(context, lightThemes[position], radiusSeekBar, radiusLabel, rectButton, ovalButton, internalUpdate);
                    updatePreviewWindow(act, root, themeName, themesLabel, shapelabel, radiusLabel, radiusSeekBar, rectButton, ovalButton, lightSpinner, darkSpinner);
                }
                @Override public void onNothingSelected(AdapterView<?> parent) {}
            };

        AdapterView.OnItemSelectedListener darkListener = new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                    if (internalUpdate[0] || !spinnerReady[0]) return;
                    selectTheme(context, darkThemes[position], radiusSeekBar, radiusLabel, rectButton, ovalButton, internalUpdate);
                    updatePreviewWindow(act, root, themeName,  themesLabel, shapelabel, radiusLabel, radiusSeekBar, rectButton, ovalButton, lightSpinner, darkSpinner);
                }
                @Override public void onNothingSelected(AdapterView<?> parent) {}
            };

        lightSpinner.setOnItemSelectedListener(lightListener);
        darkSpinner.setOnItemSelectedListener(darkListener);

        radiusSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (internalUpdate[0] || !isShapeEditorAvailable()) return;
                currentRadius = progress;
                radiusLabel.setText(act.getString(R.string.radius) + currentRadius + "dp");
                updatePreviewWindow(act, root, themeName,  themesLabel, shapelabel, radiusLabel,
                        radiusSeekBar, rectButton, ovalButton, lightSpinner, darkSpinner);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        Consumer<View> test1= buttonView  ->  {
            if (internalUpdate[0] || !isShapeEditorAvailable()) return;
            currentIsOval = buttonView == ovalButton;
            radiusSeekBar.setEnabled(!currentIsOval);
            updatePreviewWindow(act, root, themeName,  themesLabel, shapelabel, radiusLabel, radiusSeekBar, rectButton, ovalButton, lightSpinner, darkSpinner);
        };
        Object[] tests={test1};
        Backup.setradiotest(new RadioButton[]{rectButton,ovalButton},tests);

        updatePreviewWindow(act, root, themeName,  themesLabel, shapelabel, radiusLabel, radiusSeekBar, rectButton, ovalButton, lightSpinner, darkSpinner);

        root.post(() -> spinnerReady[0] = true);

        MainActivity.setonback(()-> closePanel(root,parent));

        return root;
    }

    private static void selectTheme(Context context,
                                    AppTheme theme,
                                    SeekBar radiusSeekBar,
                                    TextView radiusLabel,
                                    RadioButton rectButton,
                                    RadioButton ovalButton,
                                    boolean[] internalUpdate) {
        if (theme == null) return;

        if(selectedTheme != theme) {
            selectedTheme=theme;
            currentRadius = getDimensionFromTheme(context, selectedTheme, tk.glucodata.R.attr.buttonCornerRadius);
            Log.i(LOG_ID, "buttonCornerRadius=" + currentRadius);
            currentIsOval = getDrawableResourceFromTheme( context, selectedTheme, tk.glucodata.R.attr.themedButtonBackground) == tk.glucodata.R.drawable.button_oval;
           }

        internalUpdate[0] = true;
        radiusSeekBar.setProgress(currentRadius);
        radiusLabel.setText(context.getString(R.string.radius) + currentRadius + "dp");
        if (currentIsOval) ovalButton.setChecked(true); else rectButton.setChecked(true);

        boolean shapeEditorAvailable = theme != AppTheme.DEFAULT;
        rectButton.setEnabled(shapeEditorAvailable);
        ovalButton.setEnabled(shapeEditorAvailable);
        radiusSeekBar.setEnabled(shapeEditorAvailable && !currentIsOval);

        internalUpdate[0] = false;
    }

    private static void updatePreviewWindow(MainActivity act,
                                            Layout root,
                                            TextView themeName,
                                            TextView themesLabel,
                                            TextView shapeLabel,
                                            TextView radiusLabel,
                                            SeekBar radiusSeekBar,
                                            RadioButton rectButton,
                                            RadioButton ovalButton,
                                            Spinner lightSpinner,
                                            Spinner darkSpinner) {
        var themeContext = getSelectedThemeContext(act);
        String pkg = act.getPackageName();

        int textColor = DynamicThemeUtils.resolveAttributeColor( themeContext, android.R.attr.textColorPrimary, Color.BLACK);

//        root.setBackgroundResource(R.drawable.dialogbackground);
        Drawable bg=themeContext.getDrawable(R.drawable.dialogbackground);
//        Drawable bg = getDrawableFromTheme(themeContext, tk.glucodata.R.attr.dialogbackground);
        if(bg != null) {
            Log.i(LOG_ID,"getDrawableFromTheme(..)!=null");
            root.setBackground(bg);
            }  
         else {
            Log.i(LOG_ID,"getDrawableFromTheme(..)==null");
            int windowBg = DynamicThemeUtils.resolveAttributeColor( themeContext, android.R.attr.windowBackground, Color.WHITE);
            root.setBackgroundColor(windowBg);
            }

        themeName.setText(selectedTheme.toString());
        themeName.setTextColor(textColor);
        themesLabel.setTextColor(textColor);
        shapeLabel.setTextColor(textColor);
        radiusLabel.setTextColor(textColor);
        rectButton.setTextColor(textColor);
        ovalButton.setTextColor(textColor);

        setShapeEditorVisibility(
                isShapeEditorAvailable(),
                shapeLabel,
                radiusLabel,
                radiusSeekBar,
                rectButton,
                ovalButton
        );

        stylePreviewNativeControls(
                themeContext,
                radiusSeekBar,
                rectButton,
                ovalButton,
                lightSpinner,
                darkSpinner
        );

        notifySpinnerChanged(lightSpinner);
        notifySpinnerChanged(darkSpinner);


        Button applyButton = new Button(themeContext);
        applyButton.setText(R.string.apply);
        stylePreviewButton(themeContext, pkg, applyButton);

        applyButton.setOnClickListener(v -> applySelectedTheme(act));

        Button cancelButton = new Button(themeContext);
        cancelButton.setText(R.string.cancel);
        stylePreviewButton(themeContext, pkg, cancelButton);
        cancelButton.setOnClickListener(v -> MainActivity.doonback());
        var helpButton=getbutton(themeContext,R.string.helpname);
        helpButton.setOnClickListener(but ->  {
                act.lightBars(!selectedTheme.isDark());
                help.reset();
                help.help( act.getString(R.string.themes), themeContext, l->{
                    act.themeLightBars();
                        },(v,w,h)-> {
                if(selectedTheme != AppTheme.DEFAULT) {
                    View okbut = okbutton.get();
                    if(okbut != null) {
                        applyTheme(okbut,currentRadius,currentIsOval);
                        };
                     }
                  return new int[] {w,h};
                }, new ViewGroup.MarginLayoutParams(MATCH_PARENT, MATCH_PARENT));});

        stylePreviewButton(themeContext, pkg, helpButton);

        root.delrow(4);
        root.addrow(true,new View[]{cancelButton,helpButton,applyButton});
    }

    private static void stylePreviewNativeControls(Context themeContext,
                                                   SeekBar seekBar,
                                                   RadioButton rectButton,
                                                   RadioButton ovalButton,
                                                   Spinner lightSpinner,
                                                   Spinner darkSpinner) {
        int normal = DynamicThemeUtils.resolveAttributeColor(
                themeContext,
                android.R.attr.colorControlNormal,
                Color.GRAY
        );

        int activated = DynamicThemeUtils.resolveAttributeColor(
                themeContext,
                android.R.attr.colorControlActivated,
                DynamicThemeUtils.resolveAttributeColor(
                        themeContext,
                        android.R.attr.colorAccent,
                        normal
                )
        );

        int textColor = DynamicThemeUtils.resolveAttributeColor(
                themeContext,
                android.R.attr.textColorPrimary,
                Color.BLACK
        );

        int disabledControlAttr = themeContext.getResources().getIdentifier(
                "colorDisabledControl", "attr", themeContext.getPackageName());
        int disabledTextAttr = themeContext.getResources().getIdentifier(
                "colorDisabledText", "attr", themeContext.getPackageName());

        int disabledControl = DynamicThemeUtils.resolveAttributeColor(
                themeContext, disabledControlAttr, normal);
        int disabledText = DynamicThemeUtils.resolveAttributeColor(
                themeContext, disabledTextAttr, textColor);

        ColorStateList compoundTint = new ColorStateList(
                new int[][] {
                        new int[] { -android.R.attr.state_enabled },
                        new int[] { android.R.attr.state_checked },
                        new int[] {}
                },
                new int[] {
                        disabledControl,
                        activated,
                        normal
                }
        );

        ColorStateList compoundText = new ColorStateList(
                new int[][] {
                        new int[] { -android.R.attr.state_enabled },
                        new int[] {}
                },
                new int[] { disabledText, textColor }
        );

        rectButton.setTextColor(compoundText);
        ovalButton.setTextColor(compoundText);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            rectButton.setButtonTintList(compoundTint);
            ovalButton.setButtonTintList(compoundTint);

            ColorStateList seekActivatedTint = new ColorStateList(
                    new int[][] {
                            new int[] {-android.R.attr.state_enabled},
                            new int[] {}
                    },
                    new int[] {disabledControl, activated}
            );
            ColorStateList seekNormalTint = new ColorStateList(
                    new int[][] {
                            new int[] {-android.R.attr.state_enabled},
                            new int[] {}
                    },
                    new int[] {disabledControl, normal}
            );

            seekBar.setProgressTintList(seekActivatedTint);
            seekBar.setProgressBackgroundTintList(seekNormalTint);
            seekBar.setThumbTintList(seekActivatedTint);
            seekBar.setSecondaryProgressTintList(seekNormalTint);

            // Do not tint the native spinner shell with textColor. When AndroidDefault
            // is the active app theme, that can leave the closed/unselected spinner
            // looking dark while the preview text is dark too. Replace the shell with
            // a deterministic preview-colored background instead.
            styleSpinnerShell(themeContext, lightSpinner);
            styleSpinnerShell(themeContext, darkSpinner);

            // The popup background must be styled for the spinner group, not for the
            // currently selected preview theme. Otherwise a light-theme spinner opened
            // while AndroidDefault is active can still get a dark popup surface.
            styleSpinnerPopup(themeContext, lightSpinner, AppTheme.LIGHT);
            styleSpinnerPopup(themeContext, darkSpinner, AppTheme.DARK);
        }
    }
/*
    private static void styleSpinnerShell(Context themeContext, Spinner spinner) {
        int bgColor = DynamicThemeUtils.resolveAttributeColor(
                themeContext,
                android.R.attr.windowBackground,
                selectedTheme.isDark() ? Color.BLACK : Color.WHITE
        );

        int normal = DynamicThemeUtils.resolveAttributeColor(
                themeContext,
                android.R.attr.colorControlNormal,
                selectedTheme.isDark() ? Color.LTGRAY : Color.GRAY
        );

        float density = themeContext.getResources().getDisplayMetrics().density;
        GradientDrawable shell = new GradientDrawable();
        shell.setShape(GradientDrawable.RECTANGLE);
        shell.setColor(bgColor);
        shell.setStroke(Math.max(1, (int) density), normal);
        shell.setCornerRadius(4 * density);

        int pL = spinner.getPaddingLeft();
        int pT = spinner.getPaddingTop();
        int pR = spinner.getPaddingRight();
        int pB = spinner.getPaddingBottom();
        spinner.setBackground(shell);
        spinner.setPadding(pL, pT, pR, pB);
    } */


private static void styleSpinnerShell(Context themeContext, Spinner spinner) { }
/*
private static void styleSpinnerShell(Context themeContext, Spinner spinner) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
        spinner.setBackground(new ColorDrawable(Color.TRANSPARENT));
    } else {
        spinner.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
    }

    int windowBg = DynamicThemeUtils.resolveAttributeColor(
            themeContext,
            android.R.attr.windowBackground,
            Color.WHITE
    );

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
        spinner.setPopupBackgroundDrawable(new ColorDrawable(windowBg));
    }
  }

*/
    private static void styleSpinnerPopup(Context baseContext, Spinner spinner, AppTheme popupTheme) {
        Context popupContext = new ContextThemeWrapper(baseContext, popupTheme.getStyleResId());

        int popupBg = DynamicThemeUtils.resolveAttributeColor(
                popupContext,
                android.R.attr.windowBackground,
                popupTheme.isDark() ? Color.BLACK : Color.WHITE
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            spinner.setPopupBackgroundDrawable(new ColorDrawable(popupBg));
        }
    }

    private static void stylePreviewButton(Context themeContext, String pkg, Button button) {
        if (selectedTheme == AppTheme.DEFAULT) {
            return;
        }

        int pL = button.getPaddingLeft();
        int pT = button.getPaddingTop();
        int pR = button.getPaddingRight();
        int pB = button.getPaddingBottom();

        button.setBackground(DynamicThemeUtils.createDynamicButton(themeContext, currentRadius, currentIsOval));
        button.setPadding(pL, pT, pR, pB);

        int textAttrId = themeContext.getResources().getIdentifier("colorButtonText", "attr", pkg);
        int btnTextColor = DynamicThemeUtils.resolveAttributeColor(themeContext, textAttrId, Color.WHITE);
        int disabledTextAttr = themeContext.getResources().getIdentifier(
                "colorDisabledText", "attr", pkg);
        int disabledTextColor = DynamicThemeUtils.resolveAttributeColor(
                themeContext, disabledTextAttr, btnTextColor);
        button.setTextColor(new ColorStateList(
                new int[][] {
                        new int[] {-android.R.attr.state_enabled},
                        new int[] {}
                },
                new int[] {disabledTextColor, btnTextColor}
        ));
    }

    private static void applySelectedTheme(MainActivity act) {
        Natives.setradius(currentRadius);
        Natives.setisOval(currentIsOval);

        int globalPosition = 0;
        for (int i = 0; i < AppTheme.values().length; i++) {
            if (AppTheme.values()[i] == selectedTheme) {
                globalPosition = i;
                break;
            }
        }
        Natives.setTheme(globalPosition);
        act.recreate();
    }

    private static void closePanel(View root,View parent) {
        EnableControls(parent,true);
        if (root.getParent() instanceof ViewGroup) {
            ((ViewGroup) root.getParent()).removeView(root);
        }
    }

    private static ContextThemeWrapper getSelectedThemeContext(Context context) {
        return new ContextThemeWrapper(context, selectedTheme.getStyleResId());
    }

    private static void notifySpinnerChanged(Spinner spinner) {
        if (spinner.getAdapter() instanceof BaseAdapter) {
            ((BaseAdapter) spinner.getAdapter()).notifyDataSetChanged();
        }
    }

    private static void setSpinnerSelection(Spinner spinner, AppTheme[] themes, AppTheme theme) {
        for (int i = 0; i < themes.length; i++) {
            if (themes[i] == theme) {
                spinner.setSelection(i);
                return;
            }
        }
        spinner.setSelection(0);
    }

    public static void show(MainActivity act,View parent) {
        EnableControls(parent,false);
        int height= GlucoseCurve.getheight();
        int width= GlucoseCurve.getwidth();
        int smallest=Math.min(height,width);
        var layout = createThemeConfigurationPanel(act,parent);
        var params = new FrameLayout.LayoutParams( WRAP_CONTENT,(int)(smallest*.8f), Gravity.CENTER_HORIZONTAL | Gravity.TOP);
        /*
        var params = new FrameLayout.LayoutParams( WRAP_CONTENT,MATCH_PARENT, Gravity.CENTER_HORIZONTAL | Gravity.TOP);
        float density = act.getResources().getDisplayMetrics().density;
        params.bottomMargin=(int)(density*150.0f);
        */


    //    var params = new FrameLayout.LayoutParams( (int)(width*.5f), WRAP_CONTENT, Gravity.CENTER_HORIZONTAL | Gravity.TOP);

        act.addMyContentView(layout, params);
    }

private static int getResourceFromTheme(Context context,AppTheme apptheme, int attributeResId) {
    Resources.Theme theme = context.getResources().newTheme();
    theme.setTo(context.getTheme());
    theme.applyStyle(apptheme.getStyleResId(), true);
    var typedValue = new TypedValue();
    theme.resolveAttribute(attributeResId, typedValue, true);
    return typedValue.resourceId;
    }
private static int getDimensionFromTheme( Context context, AppTheme apptheme, int attributeResId) {
    Resources.Theme theme = context.getResources().newTheme();
    theme.setTo(context.getTheme());
    theme.applyStyle(apptheme.getStyleResId(), true);

    TypedValue typedValue = new TypedValue();

    boolean found = theme.resolveAttribute(attributeResId, typedValue, true);
    if (!found) {
        throw new IllegalArgumentException("Attribute not found in theme: " + attributeResId);
    }

    if (typedValue.type != TypedValue.TYPE_DIMENSION) {
        throw new IllegalArgumentException("Attribute is not a dimension: " + attributeResId);
    }

    return tk.glucodata.util.pxToDp(context,TypedValue.complexToDimensionPixelSize( typedValue.data, context.getResources().getDisplayMetrics()));
     }


@DrawableRes
private static int getDrawableResourceFromTheme( Context context, AppTheme appTheme, int attributeResId) {
    Resources.Theme theme = context.getResources().newTheme();
    theme.setTo(context.getTheme());
    theme.applyStyle(appTheme.getStyleResId(), true);

    TypedArray a = theme.obtainStyledAttributes(new int[]{ attributeResId });
    try {
        return a.getResourceId(0, 0);
    } finally {
        a.recycle();
    }
   }

    private static int dp(Context context, int value) {
        return (int) (value * context.getResources().getDisplayMetrics().density + 0.5f);
    }
/*
private static Drawable getDrawableFromTheme(Context context, int attrResId) {
    TypedArray a = context.obtainStyledAttributes(new int[]{ attrResId });
    try {
        Drawable d = a.getDrawable(0);
        return d != null ? d.mutate() : null;
    } finally {
        a.recycle();
    }
   }
*/
}
