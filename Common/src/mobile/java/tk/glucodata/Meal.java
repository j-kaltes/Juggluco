/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 15:32:11 CET 2023                                                 */


package tk.glucodata;

import android.os.Build;
import android.graphics.Rect;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.Button;

import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.GridLayout;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Random;

import androidx.annotation.NonNull;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import tk.glucodata.nums.numio;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Applic.isWearable;
import static tk.glucodata.Log.doLog;
import static tk.glucodata.MainActivity.doonback;
import static tk.glucodata.MainActivity.setonback;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.NumberView.geteditview;

import static tk.glucodata.NumberView.geteditwearos;
import static tk.glucodata.NumberView.smallScreen;
import static tk.glucodata.RingTones.EnableControls;
import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.edit2float;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.settings.Settings.str2float;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

class Meal {
private static final String LOG_ID="Meal";

private static int clamp(final int value,final int minimum,final int maximum) {
    if(maximum<minimum)
        return minimum;
    return Math.max(minimum,Math.min(value,maximum));
    }

private static int safeWidth(final int screenWidth) {
    return Math.max(0,screenWidth-MainActivity.systembarLeft-MainActivity.systembarRight);
    }

private static int safeHeight(final int screenHeight) {
    return Math.max(0,screenHeight-MainActivity.systembarTop-MainActivity.systembarBottom);
    }

private static int centeredSafeX(final int screenWidth,final int viewWidth) {
    return MainActivity.systembarLeft+Math.max(0,(safeWidth(screenWidth)-viewWidth)/2);
    }

private static int bottomOf(final View view) {
    if(view==null)
        return MainActivity.systembarTop;
    return Math.round(view.getY())+view.getMeasuredHeight();
    }

private interface OrientationUpdater {
    void update(View view);
    }

private static void updateOrientationLayout(final View view,final OrientationUpdater updater) {
    if(view.getParent()==null)
        return;
    updater.update(view);
    view.forceLayout();
    view.requestLayout();
    }

private static void registerOrientationLayout(final MainActivity act,final View view,final OrientationUpdater updater) {
    final View root=act.findViewById(android.R.id.content);
    if(root==null)
        return;
    final View.OnLayoutChangeListener rootListener=(changed,left,top,right,bottom,oldLeft,oldTop,oldRight,oldBottom)-> {
        if(right-left==oldRight-oldLeft&&bottom-top==oldBottom-oldTop)
            return;
        updateOrientationLayout(view,updater);
        view.post(()->updateOrientationLayout(view,updater));
        };
    root.addOnLayoutChangeListener(rootListener);
    view.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
        @Override
        public void onViewAttachedToWindow(View attached) { }

        @Override
        public void onViewDetachedFromWindow(View detached) {
            root.removeOnLayoutChangeListener(rootListener);
            detached.removeOnAttachStateChangeListener(this);
            }
        });
    }

private static void requestMenuItemLayout(final View view,final NumberView numb) {
    view.requestLayout();
    if(numb.keyboard!=null) {
        numb.keyboard.forceLayout();
        numb.keyboard.requestLayout();
        }
    }

private static int menuViewWidth(final MainActivity act) {
    int width;
    int height;
    if(Build.VERSION.SDK_INT>=Build.VERSION_CODES.R) {
        final Rect bounds=act.getWindowManager().getCurrentWindowMetrics().getBounds();
        width=bounds.width();
        height=bounds.height();
        }
    else {
        final DisplayMetrics metrics=new DisplayMetrics();
        act.getWindowManager().getDefaultDisplay().getRealMetrics(metrics);
        width=metrics.widthPixels;
        height=metrics.heightPixels;
        }
    if(width<=0||height<=0) {
        width=GlucoseCurve.getwidth(act);
        height=GlucoseCurve.getheight(act)+MainActivity.systembarTop+MainActivity.systembarBottom;
        }

    // Use stable system-bar insets rather than only the currently visible bars.
    // This makes the fixed menu width identical before and after immersive mode
    // is toggled, while still reserving the navigation-bar allocation.
    int navigationInset=Math.max(MainActivity.systembarBottom,
            Math.max(MainActivity.systembarLeft,MainActivity.systembarRight));
    final View content=act.findViewById(android.R.id.content);
    if(content!=null) {
        final WindowInsetsCompat windowInsets=ViewCompat.getRootWindowInsets(content);
        if(windowInsets!=null) {
            final Insets stableBars=windowInsets.getInsetsIgnoringVisibility(
                    WindowInsetsCompat.Type.systemBars());
            navigationInset=Math.max(stableBars.bottom,
                    Math.max(stableBars.left,stableBars.right));
            }
        }

    final int shortSide=Math.min(width,height);
    final int longSide=Math.max(width,height);
    return Math.max(1,Math.min(shortSide,(longSide-navigationInset)/2));
    }

private static FrameLayout.LayoutParams menuViewParams(final Layout lay,final NumberView numb,final MainActivity act,final int viewWidth) {
    final boolean landscape=GlucoseCurve.isLandscape(act);
    lay.setMinimumWidth(viewWidth);
    final FrameLayout.LayoutParams params;
    if(landscape) {
        params=new FrameLayout.LayoutParams(viewWidth,MATCH_PARENT,Gravity.TOP|Gravity.RIGHT);
        params.topMargin=MainActivity.systembarTop;
        params.bottomMargin=MainActivity.systembarBottom;
        params.rightMargin=MainActivity.systembarRight;
        }
    else {
        final int gap=GlucoseCurve.dpToPx(8);
        final int top=Math.max(MainActivity.systembarTop,bottomOf(numb.newnumview)+gap);
        params=new FrameLayout.LayoutParams(viewWidth,MATCH_PARENT,Gravity.TOP|Gravity.LEFT);
        params.leftMargin=centeredSafeX(GlucoseCurve.getwidth(act),viewWidth);
        params.topMargin=top;
        params.bottomMargin=MainActivity.systembarBottom;
        }
    return params;
    }

private static FrameLayout.LayoutParams selectIngredientParams(final RecyclerView recycle,final MainActivity act) {
    final int width=GlucoseCurve.getwidth(act);
    final boolean landscape=GlucoseCurve.isLandscape(act);
    recycle.setMinimumWidth((int)(safeWidth(width)*.56f));
    final FrameLayout.LayoutParams params=new FrameLayout.LayoutParams(
            smallScreen||!landscape?MATCH_PARENT:WRAP_CONTENT,
            MATCH_PARENT,
            Gravity.TOP|Gravity.CENTER_HORIZONTAL);
    params.topMargin=MainActivity.systembarTop;
    params.bottomMargin=MainActivity.systembarBottom;
    params.leftMargin=MainActivity.systembarLeft;
    params.rightMargin=MainActivity.systembarRight;
    return params;
    }

static public class MealItemViewAdapter extends RecyclerView.Adapter<MealItemViewHolder> {
    Consptr ingrindex;
    final int[] mealptrar;
    MealItemViewAdapter(final int[] mealptrar,Consptr ingrindex) {
       this.mealptrar=mealptrar;
       this.ingrindex=ingrindex;
        }

    @NonNull
    @Override
    public MealItemViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        Button view=new Button( parent.getContext());

    view.setTransformationMethod(null);
//      view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f);
   if(!isWearable)
           view.setTextSize(TypedValue.COMPLEX_UNIT_PX,Applic.mediumfontsize);
    view.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
//       view.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
//       view.setGravity(Gravity.CENTER_VERTICAL | Gravity.FILL_HORIZONTAL);
       view.setGravity(Gravity.LEFT);
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) { view.setFontFeatureSettings("pnum"); }
        return new MealItemViewHolder(view,ingrindex);
        }

    @Override
    public void onBindViewHolder(final MealItemViewHolder holder, int pos) {
        TextView text=(TextView)holder.itemView;
        float amount=Natives.getitemamount(mealptrar[0],pos);
        String ingre= Natives.getitemingredientname(mealptrar[0],pos);
        if(ingre==null|| Float.isNaN(amount))
            text.setText(R.string.error);
        else {
            StringBuilder build=new StringBuilder();
            if(amount<10)
                build.append("    ");
            else {
                if(amount<100)
                    build.append("  ");
                }
            build.append(Math.round(amount));
            build.append("   ");
            build.append(ingre);
            text.setText(build.toString());
            }
        }
        @Override
        public int getItemCount() {
                return Natives.getmealitemnr(mealptrar[0]);
            }

}
static void askround(MainActivity act,Runnable runner ,View parent) {
    EnableControls(parent,false);
    TextView label=getlabel(act,R.string.roundto);
    EditText edit=new EditText(act);edit.setText(Float.toString(Natives.getroundto()));
    edit.setInputType(InputType.TYPE_CLASS_NUMBER |InputType.TYPE_NUMBER_FLAG_DECIMAL);//| InputType.IME_FLAG_NO_FULLSCREEN);
    edit.setImeOptions( EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_DONE);
    edit.setMinEms(4);
    Button Save=getbutton(act,R.string.save);
    Button Cancel=getbutton(act,R.string.cancel);
    Layout lay=new Layout(act,(l,w,h)->{
//        int height=GlucoseCurve.getheight();
/*
        int width=GlucoseCurve.getwidth();
        l.setY(MainActivity.systembarTop);
        if(width>w)
            l.setX((width-w)/2);
        else
            l.setX(0);
*/
        return new int[]{w,h};},new View[]{label,edit}    ,new View[]{Cancel,Save});
       int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
       label.setPadding(pad,0,pad,0);
       lay.setPadding(pad,0,pad,0);
        lay.setBackgroundColor(Applic.backgroundcolor);

    var  params = new FrameLayout.LayoutParams(WRAP_CONTENT,WRAP_CONTENT,Gravity.CENTER_HORIZONTAL|Gravity.TOP);
    params.topMargin=MainActivity.systembarTop;
    params.bottomMargin=MainActivity.systembarBottom;
    params.leftMargin=MainActivity.systembarLeft;
    params.rightMargin=MainActivity.systembarRight;
    act.addMyContentView(lay, params);
    setonback(() -> {
        removeContentView(lay);
        help.hidekeyboard(act);
        EnableControls(parent,true);
        act.hideSystemUI();
        });
    Cancel.setOnClickListener(v-> {
        doonback();
        });
    Save.setOnClickListener(v-> {
        doonback();
        float round=edit2float(edit);
        Natives.setroundto(round);
        runner.run();
        });
    }

static Layout menuview(final NumberView numb, MainActivity act, int mealptr, ObjIntConsumer<Float> give,Runnable endrun) {
    RecyclerView recycle = new RecyclerView(act);
    LinearLayoutManager lin = new LinearLayoutManager(act);
    recycle.setLayoutManager(lin);

    Button add=getbutton(act,R.string.additem);
    Button Help=getbutton(act,R.string.helpname);
        Help.setOnClickListener(v-> help.helplight(tk.glucodata.R.string.mealhelp,act));
    Button roundlabel=getbutton(act,act.getString(R.string.round)+Natives.getroundto());

    recycle.setLayoutParams(new ViewGroup.LayoutParams( MATCH_PARENT , MATCH_PARENT));
    Button close=getbutton(act,R.string.closename);
    Button repeat=getbutton(act,R.string.repeat);
    float     carb=Natives.carbinmeal(mealptr);
    if(carb==0.0f)
        repeat.setVisibility(INVISIBLE);
    give.accept(carb,mealptr);
    float[] carbar={carb};
    int[] mealptrar={mealptr};
    Layout lay=new Layout(act,(l,w,h)-> {
        return new int[]{w,h};
        },new View[]{roundlabel,Help,repeat},new View[]{recycle},new View[] {add,close});
    //    lay.setY(MainActivity.systembarTop);
    roundlabel.setOnClickListener(v-> 
        askround(act,()->{
        give.accept(carbar[0],mealptrar[0]);
        roundlabel.setText(act.getString(R.string.round)+Natives.getroundto());
        },lay));
        lay.setBackgroundColor(Applic.backgroundcolor);
    lay.useMatch=true;
    final int viewWidth=menuViewWidth(act);
    act.addMyContentView(lay,menuViewParams(lay,numb,act,viewWidth));
    registerOrientationLayout(act,lay,view->
            view.setLayoutParams(menuViewParams((Layout)view,numb,act,viewWidth)));
//    act.addMyContentView(lay, smallScreen?new ViewGroup.LayoutParams(  MATCH_PARENT, (height-MainActivity.systembarTop)):new ViewGroup.LayoutParams(WRAP_CONTENT, (height-MainActivity.systembarTop-MainActivity.systembarBottom)));
    repeat.setOnClickListener(v->{
         removeContentView(lay);
         act.hideSystemUI();
         if(mealptrar[0]!=0) {
           if(numb.currentnum!=0&&numb.currentnum!= numio.newhit) {
                {if(doLog) {Log.i(LOG_ID,"repeat");};};
            Natives.hitsetmealptr(numb.currentnum,mealptrar[0]);
            }    
            int ptrcp=Natives.cpmeal(mealptrar[0]);
            act.poponback();
            numb.addnumberwithmenu(act,ptrcp);
        }
    });
    close.setOnClickListener(v-> doonback());
    setonback(() -> {
        lay.setVisibility(GONE);
        removeContentView(lay);
        act.hideSystemUI();
//        give.accept(mealptrar[0],carbar[0]);
        endrun.run();
        });
    Consptr consar=new Consptr();
    MealItemViewAdapter foodadapt = new MealItemViewAdapter(mealptrar,consar); //USE
    IntConsumer onsave= newmealptr-> {
        if(newmealptr>=0) {
            carbar[0]=Natives.carbinmeal(newmealptr);
            mealptrar[0]=newmealptr;
            give.accept(carbar[0],newmealptr);
            foodadapt.notifyDataSetChanged();
            }
        lay.setVisibility(VISIBLE);
        };
    IntConsumer hiercons=i-> {
        lay.setVisibility(GONE);
        menuitem(act,numb,mealptrar[0],i,onsave,carbar[0]);
        };
    consar.cons=hiercons;
    recycle.setAdapter(foodadapt);

    add.setOnClickListener(v-> {
        lay.setVisibility(GONE);
        menuitem(act,numb,mealptrar[0],-1,onsave,carbar[0]);
        });
    
    return lay;
    }

static void  numhidekeyboard(NumberView numb,MainActivity act) {
    if(!smallScreen)
            numb.hidekeyboard();
    else
        help.hidekeyboard(act);
    }
static void  numshowkeyboard(NumberView numb,MainActivity act) {
    numshowkeyboard(numb,act,numb.newnumview);
    }
static void  numshowkeyboard(NumberView numb,MainActivity act,View anchor) {
    if(!smallScreen)
        numb.showkeyboard(act,anchor);
    else
        help.showkeyboard(act,numb.valueedit);
    }
public static String ondecimal(final float fl,final float nr) {
    return Float.toString(Math.round(fl*nr)/nr);
    }
static void menuitem(MainActivity act, NumberView numb, int mealptr, int pos, IntConsumer give,float carbmealin) {
 
    TextView ingrlabel=getlabel(act,R.string.ingredient);
    Button Ingredient=getbutton(act,R.string.select);
    Button Save=getbutton(act,R.string.save);
    Button Cancel=getbutton(act,R.string.cancel);
    Button Delete=getbutton(act,R.string.delete);
    TextView carblabel=getlabel(act,R.string.carbohydrate);
    TextView carbos=new TextView(act);
    carbos.setPadding(0,(int)(tk.glucodata.GlucoseCurve.metrics.density*12.0),(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0),(int)(tk.glucodata.GlucoseCurve.metrics.density*9.0));

    TextView totallabel=getlabel(act,R.string.total);
    editfocus focushere=new editfocus();
    EditText total=smallScreen?geteditwearos(act):geteditview(act,focushere);
    total.setMinEms(5);
    TextView mealtotallabel=getlabel(act,R.string.mealtotal);
    EditText mealtotal=smallScreen?geteditwearos(act):geteditview(act,focushere);
    mealtotal.setMinEms(5);
    TextView amountlabel=getlabel(act,R.string.quantity);
    EditText amount    = smallScreen?geteditwearos(act):geteditview(act,focushere);


    amount.requestFocus();

    editfocus.setedittext(amount);
    amount.setMinEms(5);
    float[] cargs={0.0f};
    int [] ingred={-1};
    if(pos>=0) {
         float am=Natives.getitemamount( mealptr, pos);
         amount.setText(""+Math.round(am*10.0f)/10.0f);
        int ingr=Natives.getitemingredient(mealptr, pos);
         ingred[0]=ingr;
        Ingredient.setText(Natives.ingredientName(ingr));
        String unit=Natives.ingredientUnitName(ingr);
        float carb=Natives.ingredientCarb(ingr);
        carbos.setText(carb+act.getString(R.string.per)+unit);
        cargs[0]=carb;
         float tot=am*carb;
         total.setText(ondecimal(tot,10));
         mealtotal.setText(ondecimal(carbmealin,10));
         carbmealin-=tot;

        }
    else
        Delete.setVisibility(INVISIBLE);
    final float carbmeal=carbmealin;
    boolean[] changing={false};
    amount.addTextChangedListener( new TextWatcher() {
           public void afterTextChanged(Editable ed) {
               if(!changing[0]) {
                changing[0]=true;
                 float am=str2float( ed.toString());
                 float tot=am*cargs[0];
                total.setText(ondecimal(tot,10));
                mealtotal.setText(ondecimal(carbmeal+tot,10));
                changing[0]=false;
                }
           }
           public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
           public void onTextChanged(CharSequence s, int start, int before, int count) { }
          });
    total.addTextChangedListener( new TextWatcher() {
           public void afterTextChanged(Editable ed) {
               if(!changing[0]) {
                if(cargs[0]>0.0f) {
                    changing[0]=true;
                     float tot=str2float( ed.toString());
                     float am=tot/cargs[0];
                    amount.setText(ondecimal(am,10));
                    mealtotal.setText(ondecimal(carbmeal+tot,10));
                    changing[0]=false;
                    }
                }
               }
           public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
           public void onTextChanged(CharSequence s, int start, int before, int count) { }
          });
    mealtotal.addTextChangedListener( new TextWatcher() {
           public void afterTextChanged(Editable ed) {
               if(!changing[0]) {
                if(cargs[0]>0.0f) {
                    changing[0]=true;
                     float mealtot=str2float( ed.toString());
                     float tot=mealtot-carbmeal;
                     float am=tot/cargs[0];
                    amount.setText(ondecimal(am,10));
                    total.setText(ondecimal(tot,10));
                    changing[0]=false;
                    }
                }
               }
           public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
           public void onTextChanged(CharSequence s, int start, int before, int count) { }
          });
    Layout lay=new Layout(act,(l,w,h)-> {
        int width=GlucoseCurve.getwidth(act);
        int hei=GlucoseCurve.getheight(act);
        Log.i(LOG_ID,"addcomponent width="+width+" height="+hei+" w="+w+" h="+h);
        if(!smallScreen&&GlucoseCurve.isLandscape(act)) {
            int leftHalf=MainActivity.systembarLeft+safeWidth(width)/2;
            int xpos=leftHalf-w;
            l.setX(Math.max(MainActivity.systembarLeft,xpos));
            int maximumY=hei-MainActivity.systembarBottom-h;
            int centered=MainActivity.systembarTop+(safeHeight(hei)-h)/2;
            l.setY(clamp(centered,MainActivity.systembarTop,maximumY));
            }
        else {
            l.setX(centeredSafeX(width,w));
            l.setY(MainActivity.systembarTop);
            }
        return new int[]{w,h};
        },new View[]{amountlabel,amount},new View[]{ingrlabel,Ingredient},new View[]{carblabel,carbos},new View[]{totallabel,total}, new View[]{mealtotallabel,mealtotal},
            new View[] {Delete,Cancel,Save});
       int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
       lay.setPadding(pad,0,pad,0);
    act.addMyContentView(lay, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    registerOrientationLayout(act,lay,view->requestMenuItemLayout(view,numb));
      lay.post(() -> {
          lay.requestLayout();
          numshowkeyboard(numb,act,lay);
          });

        lay.setBackgroundColor(Applic.backgroundcolor);

    Delete.setOnClickListener(v-> {
        int newmealptr=Natives.deletefrommeal(mealptr,pos);
        give.accept(newmealptr);
        removeContentView(lay);
            numhidekeyboard(numb,act);
        act.hideSystemUI();
        act.poponback();
        });
    Cancel.setOnClickListener(v-> doonback());
     setonback(() -> {
        lay.setVisibility(GONE);
        give.accept(-1);
        removeContentView(lay);
            numhidekeyboard(numb,act);
        act.hideSystemUI();
         });
    Ingredient.setOnClickListener(v-> {
        
        selectingredient(act,numb,lay,i->{
    if(i<0) {
        if(ingred[0]>=0&&i==(-ingred[0]-1)) {
            ingred[0]=-1;
            Ingredient.setText(R.string.select);
            }
        }
    else {
        ingred[0] = i;
        Ingredient.setText(Natives.ingredientName(i));
        String unit = Natives.ingredientUnitName(i);
        float carb = Natives.ingredientCarb(i);
        carbos.setText(carb + act.getString(R.string.per) + unit);
        cargs[0] = carb;
        float am = str2float(amount.getText().toString());
        float tot = am * carb;
        total.setText(ondecimal(tot, 10));
    }
        editfocus.getedittext().requestFocus();
        });});
    Save.setOnClickListener(v-> {
        if(ingred[0]==-1) {
            Applic.argToaster(act, R.string.specifyingredient, Toast.LENGTH_SHORT);
            return;
            }
         float am=str2float( amount.getText().toString());
         if(am==0.0f) {
            Applic.argToaster(act, R.string.specifyhowmuch, Toast.LENGTH_SHORT);
            return;
             }    
        int newmealptr=Natives.changemealitem(mealptr,pos,ingred[0],am);
        lay.setVisibility(GONE);
        removeContentView(lay);
            numhidekeyboard(numb,act);
        act.hideSystemUI();
        give.accept(newmealptr);
        act.poponback();
        });
    }

static public class IngredientViewAdapter extends RecyclerView.Adapter<IngredientViewHolder> {
    int[] results=null;
    Consptr ingrindex;
    IngredientViewAdapter(Consptr ingrindex) {
        this.ingrindex=ingrindex;
        }
    void setResults(int[] res) {
        results=res;
        notifyDataSetChanged();
        }
    @NonNull
    @Override
    public IngredientViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        Button view=new Button( parent.getContext());
  //      view.setTransformationMethod(null);
       if(!isWearable)
               view.setTextSize(TypedValue.COMPLEX_UNIT_PX,Applic.mediumfontsize);
        ViewGroup.LayoutParams params= new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        view.setLayoutParams(params);
        view.setGravity(Gravity.CENTER_VERTICAL | Gravity.LEFT);
        return new IngredientViewHolder(view,this);

    }
    @Override
    public void onBindViewHolder(final IngredientViewHolder holder, int pos) {
        int showpos=results==null?pos:results[pos];
        TextView text=(TextView)holder.itemView;
        text.setText(Natives.ingredientName(showpos));
        }
        @Override
        public int getItemCount() {
            if(results!=null)
                return results.length;
            return Natives.ingredientNr();
            }

    }
static private void doSearchIngr(MainActivity act,View view,IngredientViewAdapter adapt) {
    act.showSystemUI();
    view.setVisibility(INVISIBLE);
    var ingredient=getlabel(act,R.string.ingredient);
    EditText searchstr= new EditText(act);
    searchstr.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
    int searchoptions=EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_SEARCH;
    searchstr.setImeOptions(searchoptions);
    searchstr.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    var cancel=getbutton(act,R.string.cancel);
    var search=getbutton(act,R.string.search);
    ingredient.setPaddingRelative((int)(tk.glucodata.GlucoseCurve.metrics.density*5.0),0,0,0);
    boolean landscape=GlucoseCurve.isLandscape(act);
    Object[][] searchRows=landscape?
            new Object[][]{new View[]{ingredient,searchstr,search,cancel}}:
            new Object[][]{new View[]{ingredient},new View[]{searchstr},new View[]{search,cancel}};
    var lay=new Layout(act,(l,w,h)-> {
    /*
        l.setY(MainActivity.systembarTop*.75f);
        l.setX(MainActivity.systembarLeft);
        w=width-MainActivity.systembarLeft-MainActivity.systembarRight;
        */
        return new int[]{w,h};
        },searchRows);
    lay.useMatch=true;
    lay.setBackgroundResource(R.drawable.helpbackground);

    setonback(() -> {
        view.setVisibility(VISIBLE);
        removeContentView(lay);
        act.hideSystemUI();
        hidekeyboard(act);
        });
    cancel.setOnClickListener(v->  doonback());

    //act.addMyContentView(lay, new ViewGroup.LayoutParams(width-MainActivity.systembarLeft-MainActivity.systembarRight ,WRAP_CONTENT));

    var  params = new FrameLayout.LayoutParams(MATCH_PARENT,WRAP_CONTENT,Gravity.TOP|Gravity.START);
    params.topMargin=MainActivity.systembarTop;
    params.leftMargin=MainActivity.systembarLeft;
    params.rightMargin=MainActivity.systembarRight;
   act.addMyContentView(lay, params);
    Runnable searchrun=()-> {
                int[] res=Natives.searchIngredient(searchstr.getText().toString());
                if(res!=null) {
                    adapt.setResults(res);
                    doonback();
                    }
                 else {
                    Applic.argToaster(act, "Regex error, try again", Toast.LENGTH_LONG);
                    }
                    };
    searchstr.setOnEditorActionListener(new TextView.OnEditorActionListener() {
        @Override
        public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
            if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
                                || actionId == EditorInfo.IME_ACTION_SEARCH) {
                searchrun.run();
                return true;
               }
            return false;
            }
        });

    search.setOnClickListener(v->  
             searchrun.run()
            );
    }
static private void selectingredient(MainActivity act,NumberView numb,View keyboardAnchor,IntConsumer setindex) {
    numhidekeyboard(numb,act);
    RecyclerView recycle = new RecyclerView(act);
    LinearLayoutManager lin = new LinearLayoutManager(act);
    recycle.setLayoutManager(lin);
    var add=getbutton(act,R.string.define);
    Button close=getbutton(act,R.string.closename);
    CheckDirectionBox edit=new CheckDirectionBox(act);
    edit.setText(R.string.edit);
    var search=getbutton(act,R.string.search);
    recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, MATCH_PARENT));
    boolean landscape=GlucoseCurve.isLandscape(act);
//    int ypos=MainActivity.systembarTop*3/4;
    Object[][] ingredientRows=landscape?
            new Object[][]{new View[]{recycle},new View[]{add,edit,search,close}}:
            new Object[][]{new View[]{recycle},new View[]{add,edit},new View[]{search,close}};
    Layout lay=new Layout(act,(l,w,h)-> {
    /*
      var af=MainActivity.systembarTop*3/4;
        l.setY(af);
       l.setX((width-w)/2);
        return new int[]{w,h-af-MainActivity.systembarBottom};
       */
        return new int[]{w,h};
        },ingredientRows);
    lay.useMatch=true;

//    lay.setY(ypos);
//    lay.setMinimumWidth(viewwidth);
    final int wasonback=MainActivity.onbacknr();
    IntConsumer hiercons=i-> {
        /*lay.setVisibility(GONE);
        removeContentView(lay);*/
        while( MainActivity.onbacknr()>wasonback) doonback();
        setindex.accept(i);
        };
    Consptr consar=new Consptr(hiercons);
    IngredientViewAdapter foodadapt = new IngredientViewAdapter(consar); //USE
    recycle.setAdapter(foodadapt);
    search.setOnClickListener(v-> doSearchIngr(act,search,foodadapt));

    edit.setOnCheckedChangeListener((buttonView, isChecked)-> {
        if(isChecked) {
            consar.cons=i-> {
                edit.setChecked(false);
                defineingredient(act,foodadapt,recycle,i,setindex,lay);
                };
            }
        else
            consar.cons=hiercons;
        });


    lay.setBackgroundResource(R.drawable.dialogbackground);

//    act.addMyContentView(lay, new ViewGroup.LayoutParams(smallScreen?MATCH_PARENT:WRAP_CONTENT, height));
//    params.topMargin=0;
    act.addMyContentView(lay,selectIngredientParams(recycle,act));
    registerOrientationLayout(act,lay,view->
            view.setLayoutParams(selectIngredientParams(recycle,act)));
    lay.invalidate();
    lay.setVisibility(VISIBLE);
    lay.bringToFront();
    close.setOnClickListener(v-> doonback());
    setonback(() -> {
        numshowkeyboard(numb,act,keyboardAnchor);
        lay.setVisibility(GONE);
        removeContentView(lay);
        act.hideSystemUI();
        });
    add.setOnClickListener(v-> defineingredient(act,foodadapt,recycle,-1,setindex,lay));
    
    }
static void    defineingredient(MainActivity act ,IngredientViewAdapter  foodadapt,RecyclerView recycle,int pos, IntConsumer  setindex,View parent) {

    EnableControls(parent,false);
    recycle.suppressLayout(true);
    act.showSystemUI();
    act.showui=true;
    TextView.OnEditorActionListener     actlist= new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
            if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
                    || actionId == EditorInfo.IME_ACTION_DONE) {
        //        act.hideSystemUI();
                 tk.glucodata.help.hidekeyboard(act);
                 {if(doLog) {Log.i(LOG_ID,"onEditorAction");};};
// hidekeyboard();
                return true;
               }
            return false;
            }};

    TextView namelabel=getlabel(act,R.string.name);
    EditText name=new EditText(act);
        name.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        name.setImeOptions(editoptions);
        name.setMinEms(10);

    TextView unitlabel=getlabel(act,R.string.unit);
    EditText unit=new EditText(act);
        unit.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        unit.setImeOptions(editoptions);
        unit.setMinEms(2);
    TextView carblabel=getlabel(act,R.string.carbperunit);
    EditText carb=new EditText(act);
    carb.setMinEms(5);
    carb.setInputType(InputType.TYPE_CLASS_NUMBER |InputType.TYPE_NUMBER_FLAG_DECIMAL);//| InputType.IME_FLAG_NO_FULLSCREEN);
//    carb.setKeyListener(DigitsKeyListener.getInstance("0123456789^*/+-()."));
    carb.setImeOptions( EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_DONE);
        name.setOnEditorActionListener(actlist);
        unit.setOnEditorActionListener(actlist);
        carb.setOnEditorActionListener(actlist);
    Button Save=getbutton(act,R.string.save);
    Button Cancel=getbutton(act,R.string.cancel);
    Button Delete=getbutton(act,R.string.delete);
    Button Database=getbutton(act,R.string.database);
    Database.setOnClickListener(v-> {
        final Layout[] fooddat={null};
        fooddat[0]=fooddatabase(act,(n,val,u)-> {
            removeContentView(fooddat[0]);
            name.setText(n);
            unit.setText(u);
            carb.setText(Float.toString(val));
            });

        });
    int usedunit=0;
    if(pos>=0) {
        carb.setText(""+Natives.ingredientCarb(pos));
        usedunit=Natives.ingredientUnit(pos)+1;
        name.setText(Natives.ingredientName(pos));
        if(!Natives.ingredientdeleteable(pos))
             Delete.setVisibility(GONE);
        }
    else {
        Delete.setVisibility(GONE);
         }
    ArrayList<String> unitstr=Natives.getunits();
        Spinner spinner=new Spinner(act);
    if(unitstr!=null&&unitstr.size()>0) {
        final int minheight= GlucoseCurve.dpToPx(48);
        spinner.setMinimumHeight(minheight);
        avoidSpinnerDropdownFocus(spinner);
        LabelAdapter<String> numspinadapt=new LabelAdapter<String>(act,unitstr,0);
        spinner.setAdapter(numspinadapt);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public  void onItemSelected (AdapterView<?> parent, View view, int position, long id) {
                unit.setText(unitstr.get(position));        
                }
            @Override
            public  void onNothingSelected (AdapterView<?> parent) {
            } });
    //    spinner.clearAnimation();
        spinner.setSelection(usedunit);
        }
    else
        spinner.setVisibility(GONE);
    boolean landscape=GlucoseCurve.isLandscape(act);
    Object[][] ingredientEditRows=landscape?
            new Object[][]{
                    new View[]{namelabel,name,unitlabel,unit,spinner},
                    new View[]{Database,carblabel,carb},
                    new View[]{Cancel,Delete,Save}}:
            new Object[][]{
                    new View[]{namelabel,name},
                    new View[]{unitlabel,unit,spinner},
                    new View[]{Database,carblabel,carb},
                    new View[]{Cancel,Delete,Save}};
    Layout inlay=new Layout(act,(l,w,h)-> {
//        int height=GlucoseCurve.getheight();
/*
        int width=GlucoseCurve.getwidth();
        if(width>w)
            l.setX((width-w)/2);
        else
            l.setX(0);
        l.setY(MainActivity.systembarTop);
        */
        return new int[]{w,h};
        },ingredientEditRows);
    final var lay= new HorizontalScrollView(act);
    lay.addView(inlay, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT) );
        lay.setSmoothScrollingEnabled(false);
       lay.setVerticalScrollBarEnabled(false);
        lay.setHorizontalScrollBarEnabled(Applic.horiScrollbar);
        lay.setFillViewport(true);

       int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
       lay.setPadding(pad,0,pad,pad/2);
      lay.setBackgroundResource(R.drawable.dialogbackground);
   // act.addMyContentView(lay, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
    var  params = new FrameLayout.LayoutParams(
            landscape?WRAP_CONTENT:MATCH_PARENT,
            WRAP_CONTENT,
            Gravity.CENTER_HORIZONTAL|Gravity.TOP);
    params.topMargin=MainActivity.systembarTop;
    params.bottomMargin=MainActivity.systembarBottom;
    params.leftMargin=MainActivity.systembarLeft;
    params.rightMargin=MainActivity.systembarRight;
     act.addMyContentView(lay, params);

    name.requestFocus();
    Save.setOnClickListener(v-> {
        final float flcarb=edit2float(carb);
        Natives.saveingredient(pos,name.getText().toString(), unit.getText().toString(),flcarb);

        foodadapt.notifyDataSetChanged();
        lay.setVisibility(GONE);
        removeContentView(lay);
        recycle.suppressLayout(false);
        EnableControls(parent,true);
        act.showui=false;
        act.hideSystemUI();
        act.poponback();
        if(pos<0)
            recycle.scrollToPosition(foodadapt.getItemCount()-1);
        });
    Runnable endproc= ()-> {
        lay.setVisibility(GONE);
        removeContentView(lay);
        recycle.suppressLayout(false);
        EnableControls(parent,true);
        act.showui=false;
        act.hideSystemUI();
        };
    Cancel.setOnClickListener(v-> {
        endproc.run();
        act.poponback();
        });
    
    setonback(endproc);
    Delete.setOnClickListener(v-> {
        if(pos>=0) {
            Natives.deleteingredient(pos);
            setindex.accept(-pos-1);
            }
        foodadapt.notifyDataSetChanged();
        endproc.run();
        act.poponback();
        });

    }

static public class MealDatabaseViewAdapter extends RecyclerView.Adapter<MealDatabaseViewHolder> {
    long[] hitptr;
    TriConsumer<String,Float,String> give;
    MealDatabaseViewAdapter(long[] hitptr,TriConsumer<String,Float,String> give) {
       this.hitptr=hitptr;
       this.give=give;
        }

    @Override
    public MealDatabaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        Button view=new Button( parent.getContext());
    view.setTransformationMethod(null);
   //     view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f);

   if(!isWearable)
           view.setTextSize(TypedValue.COMPLEX_UNIT_PX,Applic.mediumfontsize);
    view.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
       view.setGravity(Gravity.LEFT);
        return new MealDatabaseViewHolder(view,hitptr,give);
        }

    @Override
    public void onBindViewHolder(final MealDatabaseViewHolder holder, int pos) {
        TextView text=(TextView)holder.itemView;
        String label=hitptr[0]==0L?Natives.idfoodlabel(pos):Natives.foodlabel(hitptr[0],pos);
        text.setText(label);
        }
        @Override
        public int getItemCount() {
            if(hitptr[0]==0L)
                return Natives.foodnr();;
            return Natives.foodhitnr( hitptr[0]);
             }

}
final static private Random random=new Random();
static Layout  fooddatabase(MainActivity act, TriConsumer<String,Float,String> give) {
    act.themeLightBars();
    RecyclerView recycle = new RecyclerView(act);
    LinearLayoutManager lin = new LinearLayoutManager(act);
    recycle.setLayoutManager(lin);
    long[] hitptr    ={0L};
    MealDatabaseViewAdapter foodadapt = new MealDatabaseViewAdapter(hitptr,give); //USE
    recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    recycle.setAdapter(foodadapt);
    Button searchbutton=getbutton(act,R.string.search);
    EditText searchstr= new EditText(act);
    searchstr.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
    int searchoptions=EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_SEARCH;
    searchstr.setImeOptions(searchoptions);
    searchstr.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    searchstr.setOnEditorActionListener(new TextView.OnEditorActionListener() {
        @Override
        public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
                            || actionId == EditorInfo.IME_ACTION_SEARCH) {
            hidekeyboard(act);
            hitptr[0]=Natives.foodsearch(searchstr.getText().toString());
            foodadapt.notifyDataSetChanged();
            return true;
           }
        return false;
        }
    });


    Button Close=getbutton(act,R.string.closename);
    Button Help=getbutton(act,R.string.helpname);
//    act.hideSystemUI();
    int fnr=Natives.foodnr();
    recycle.scrollToPosition(random.nextInt(fnr));
    // The header is rebuilt when the orientation changes.  In landscape all
    // controls fit on one line; in portrait the actions and search controls use
    // two compact lines.  Reusing the same child hierarchy without rebuilding
    // it left portrait measurements and widths behind after rotation.
    final LinearLayout controls=new LinearLayout(act);
    controls.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT,WRAP_CONTENT));

    final LinearLayout actionRow=new LinearLayout(act);
    actionRow.setOrientation(LinearLayout.HORIZONTAL);
    actionRow.setGravity(Gravity.CENTER_VERTICAL);
    actionRow.setLayoutParams(new LinearLayout.LayoutParams(MATCH_PARENT,WRAP_CONTENT));
    final View actionSpacer=new View(act);

    final LinearLayout searchRow=new LinearLayout(act);
    searchRow.setOrientation(LinearLayout.HORIZONTAL);
    searchRow.setGravity(Gravity.CENTER_VERTICAL);
    searchRow.setLayoutParams(new LinearLayout.LayoutParams(MATCH_PARENT,WRAP_CONTENT));

    final Runnable configureControls=()-> {
        // Layout.useMatch replaces MATCH_PARENT with the measured pixel width.
        // Restore it before every reconfiguration so the new orientation is
        // measured from the current full screen rather than the previous one.
        controls.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT,WRAP_CONTENT));

        // Remove every control from its old parent before constructing the new
        // orientation-specific hierarchy.
        controls.removeAllViews();
        actionRow.removeAllViews();
        searchRow.removeAllViews();

        if(GlucoseCurve.isLandscape(act)) {
            controls.setOrientation(LinearLayout.HORIZONTAL);
            controls.setGravity(Gravity.CENTER_VERTICAL);
            controls.addView(Help,new LinearLayout.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));
            controls.addView(searchstr,new LinearLayout.LayoutParams(0,WRAP_CONTENT,1.0f));
            controls.addView(searchbutton,new LinearLayout.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));
            controls.addView(Close,new LinearLayout.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));
            }
        else {
            controls.setOrientation(LinearLayout.VERTICAL);
            controls.setGravity(Gravity.NO_GRAVITY);

            actionRow.addView(Help,new LinearLayout.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));
            actionRow.addView(actionSpacer,new LinearLayout.LayoutParams(0,0,1.0f));
            actionRow.addView(Close,new LinearLayout.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));

            searchRow.addView(searchstr,new LinearLayout.LayoutParams(0,WRAP_CONTENT,1.0f));
            searchRow.addView(searchbutton,new LinearLayout.LayoutParams(WRAP_CONTENT,WRAP_CONTENT));

            controls.addView(actionRow);
            controls.addView(searchRow);
            }
        controls.forceLayout();
        controls.requestLayout();
        };
    configureControls.run();

    final Object[][] databaseRows={new View[]{controls},new View[]{recycle}};
    Layout lay=new Layout(act,(l,w,h)->{
    /*
        int width=GlucoseCurve.getwidth();
//        l.setY(MainActivity.systembarTop);
        if(width>w) {
            l.setX((width-w)/2);
                 }
        else
            l.setX(0);
            */
    //    return new int[]{w,h-MainActivity.systembarTop};
        return new int[]{w,h};

        },databaseRows);
      lay.useMatch=true;
     //  int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
    //   lay.setPadding(pad,0,pad,0);
      lay.setPadding(MainActivity.systembarLeft,MainActivity.systembarTop,MainActivity.systembarRight,MainActivity.systembarBottom);
        lay.setBackgroundColor(Applic.backgroundcolor);
    //act.addMyContentView(lay, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
/*
    var  params =    new FrameLayout.LayoutParams( WRAP_CONTENT, WRAP_CONTENT, Gravity.CENTER_HORIZONTAL);
    params.topMargin=MainActivity.systembarTop;
   context.addMyContentView(layout, params);
   */
    act.addMyContentView(lay,new FrameLayout.LayoutParams(
            MATCH_PARENT,MATCH_PARENT,Gravity.FILL));
    registerOrientationLayout(act,lay,view-> {
        configureControls.run();
        // Replace any dimensions retained from the previous orientation.  In
        // particular this prevents a portrait measurement becoming the dialog
        // width after rotating to landscape.
        view.setLayoutParams(new FrameLayout.LayoutParams(
                MATCH_PARENT,MATCH_PARENT,Gravity.FILL));
        view.setX(0.0f);
        view.setY(0.0f);
        view.setPadding(MainActivity.systembarLeft,MainActivity.systembarTop,
                MainActivity.systembarRight,MainActivity.systembarBottom);
        recycle.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT,WRAP_CONTENT));
        recycle.forceLayout();
        recycle.requestLayout();
        });
    searchstr.requestFocus();
    Help.setOnClickListener(v-> { 
//        act.hideSystemUI();
//        help.help(R.string.searchhelp,act);
        help.help(R.string.nutrients,act);
        });
    Close.setOnClickListener(v-> { 
        doonback();
        });
    setonback(() -> {
        removeContentView(lay);
    //    act.hideSystemUI();
        Natives.freefoodptr(hitptr[0]);
        act.lightBars(!Natives.getInvertColors());

        });
    searchbutton.setOnClickListener(v-> { 
        hidekeyboard(act);
    //    act.hideSystemUI();
//        Natives.freefoodptr(hitptr[0]);
        hitptr[0]=Natives.foodsearch(searchstr.getText().toString());
        foodadapt.notifyDataSetChanged();
        });
    return lay;
    }
static String[] compnames=Natives.getcomponentlabels( );
static String[] compunits=Natives.getcomponentunits( );
static void    shownutrients(MainActivity act,int id,boolean showzero,TriConsumer<String,Float,String> give) {
    hidekeyboard(act);
//    act.hideSystemUI();
    TextView ingred=new TextView(act);
    final String label= Natives.idfoodlabel(id);
    ingred.setText(label);
//        ingred.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);
   if(!isWearable)
           ingred.setTextSize(TypedValue.COMPLEX_UNIT_PX,Applic.largefontsize);
    int[] ingr=Natives.getcomponents(id);
    int rows=2;
    for(int el:ingr) {
        if((showzero&&el!=-1)||el>0)
            rows++;
        }
    GridLayout grid=new GridLayout(act);
      grid.setLayoutDirection(View.LAYOUT_DIRECTION_LTR);
      grid.setTextDirection(View.TEXT_DIRECTION_LTR);
        int cols=4;
        grid.setColumnCount(cols);
        grid.setRowCount(rows);
    GridLayout.LayoutParams params = new GridLayout.LayoutParams();
    params.columnSpec = GridLayout.spec(0, 4); 
//    ingr.setLayoutParams(params);
    grid.addView(ingred,params);

    GridLayout.LayoutParams para = new GridLayout.LayoutParams();
    para.columnSpec = GridLayout.spec(1, 3); 
    TextView per100=getlabel(act,R.string.compositionof100gram);
    grid.addView(per100,para);
   int pad=(int)(GlucoseCurve.metrics.density*5.0);
        for(int i=0;i<ingr.length;i++) {
        if((showzero&&ingr[i]!=-1)||ingr[i]>0) {
            TextView name = new TextView(act);
            name.setText(compnames[i]);
            name.setPadding(pad,0,pad,0);
            GridLayout.LayoutParams par = new GridLayout.LayoutParams();
            par.columnSpec = GridLayout.spec(0, 2);
            grid.addView(name,par);
//Tr -3
//N  -2
//"" -1
//0.0 0

            String strval;
            switch(ingr[i]) {
                case -3: strval=act.getString(R.string.trace);break;
                case -2: strval=act.getString(R.string.unknown);break;
                default:  {
                    float val=(float)ingr[i]/1000.0f;
                    strval=val<0.1f?Float.toString(val):ondecimal(val,10);
                    };
                }
            TextView value=new TextView(act);
            value.setText(strval);
            grid.addView(value);
            TextView un=new TextView(act);
            if(ingr[i]>=0)
                un.setText(compunits[i]);
            grid.addView(un);
            }
        }
        CheckDirectionBox zero=new CheckDirectionBox(act);
        zero.setText(R.string.showzero);
        zero.setChecked(showzero);
       zero.setPadding(0,0,0,0);
    GridLayout.LayoutParams parzero = new GridLayout.LayoutParams();
 int padzero=(int)    (GlucoseCurve.metrics.density*7.0);
    parzero.setMargins(padzero, 0, padzero, 0);
    ScrollView scroll=new ScrollView(act);
   /* 
    grid.setmeasure((l,w,h)-> {
        int height=GlucoseCurve.getheight();
        int width=GlucoseCurve.getwidth();
      int x=width>w?((width-w)/2):0;
      scroll.setX(x);
        if(!smallScreen&&height>h) {
            scroll.setY((height-h)/2);
         }
        else   {
          var above=MainActivity.systembarTop*3/4;
          scroll.setY(above);
         if((height-above)>h) 
            scroll.setPadding(0,0,0,0);
         else
                  scroll.setPadding(0,0,0,above);
        }

            });
            */
    //      scroll.setPadding(0,0,0,MainActivity.systembarTop*3/4);

    Button Select=getbutton(act,R.string.select);
    grid.addView(Select);
    zero.setOnCheckedChangeListener((buttonView, isChecked)-> {
        doonback();
//        act.hideSystemUI();
        shownutrients(act,id, isChecked,give) ;
        return;
        });
    grid.addView(zero,parzero);
    Select.setOnClickListener(v-> {
        removeContentView(scroll);
//        act.hideSystemUI();
        act.poponback();
        act.poponback();
        give.accept(label,ingr[0]/100000.0f,compunits[0]);
        });
    Button Close=getbutton(act,R.string.closename);
    GridLayout.LayoutParams closeparams = new GridLayout.LayoutParams();
    closeparams.columnSpec = GridLayout.spec(2, 2);
    grid.addView(Close,closeparams);
//    act.addMyContentView(grid,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
//    scroll.setFillViewport(true);
       grid.setPadding(pad,0,0,0);
    scroll.addView(grid);
    scroll.setSmoothScrollingEnabled(false);
        scroll.setVerticalScrollBarEnabled(Applic.scrollbar);



    //act.addMyContentView(scroll,new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));

    var  params2 = new FrameLayout.LayoutParams(WRAP_CONTENT,WRAP_CONTENT,Gravity.CENTER_HORIZONTAL|Gravity.TOP);
    params2.topMargin=MainActivity.systembarTop;
    params2.bottomMargin=MainActivity.systembarBottom;
    params2.leftMargin=MainActivity.systembarLeft;
    params2.rightMargin=MainActivity.systembarRight;
   act.addMyContentView(scroll, params2);
    scroll.setBackgroundResource(R.drawable.dialogbackground);
    Close.setOnClickListener(v-> doonback());
    setonback(() -> {
        removeContentView(scroll);
//        act.hideSystemUI();
        });

}
/*
static private String getIngredientName(int pos) {
    byte[] bytes=Natives.ingredientNameBytes(pos);
    try {
      return new String(bytes);
      }
      catch(Throwable th) {
         Log.stack(LOG_ID,"getIngredientName("+pos+")",th);
         return "illegal character";
         }
   }
*/
} 
