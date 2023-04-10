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

import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.GridLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Random;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import tk.glucodata.nums.numio;

import static android.view.View.GONE;
import static android.view.View.INVISIBLE;
import static android.view.View.VISIBLE;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.NumberView.avoidSpinnerDropdownFocus;
import static tk.glucodata.NumberView.geteditview;

import static tk.glucodata.help.hidekeyboard;
import static tk.glucodata.settings.Settings.edit2float;
import static tk.glucodata.settings.Settings.editoptions;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.settings.Settings.str2float;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

class Meal {
private static final String LOG_ID="Meal";
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
		view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f);
	view.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
//       view.setGravity(Gravity.CENTER_VERTICAL | Gravity.START);
//       view.setGravity(Gravity.CENTER_VERTICAL | Gravity.FILL_HORIZONTAL);
       view.setGravity(Gravity.LEFT);
//		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) { view.setFontFeatureSettings("pnum"); }
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
static void askround(MainActivity act,Runnable runner ) {
	TextView label=getlabel(act,R.string.roundto);
	EditText edit=new EditText(act);edit.setText(Float.toString(Natives.getroundto()));
	edit.setInputType(InputType.TYPE_CLASS_NUMBER |InputType.TYPE_NUMBER_FLAG_DECIMAL);//| InputType.IME_FLAG_NO_FULLSCREEN);
	edit.setImeOptions( EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_DONE);
	edit.setMinEms(4);
	Button Save=getbutton(act,R.string.save);
	Button Cancel=getbutton(act,R.string.cancel);
	Layout lay=new Layout(act,(l,w,h)->{
//		int height=GlucoseCurve.getheight();
		int width=GlucoseCurve.getwidth();
		l.setY(0);
		if(width>w)
			l.setX((width-w)/2);

		return new int[]{w,h};},new View[]{label,edit}	,new View[]{Cancel,Save});
	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
	   label.setPadding(pad,0,pad,0);
	   lay.setPadding(pad,0,pad,0);
    	lay.setBackgroundColor(Applic.backgroundcolor);
	act.addContentView(lay, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
	Cancel.setOnClickListener(v-> {
		removeContentView(lay);
		act.hideSystemUI();
		});
	Save.setOnClickListener(v-> {
		removeContentView(lay);
		act.hideSystemUI();
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
        Help.setOnClickListener(v-> help.help(tk.glucodata.R.string.mealhelp,act));
	Button roundlabel=getbutton(act,act.getString(R.string.round)+Natives.getroundto());

//	recycle.setLayoutParams(new ViewGroup.LayoutParams( WRAP_CONTENT , ViewGroup.LayoutParams.WRAP_CONTENT));
	recycle.setLayoutParams(new ViewGroup.LayoutParams( MATCH_PARENT , MATCH_PARENT));
	Button close=getbutton(act,R.string.closename);
	Button repeat=getbutton(act,R.string.repeat);
	float 	carb=Natives.carbinmeal(mealptr);
//	TextView total=getlabel(act,ondecimal(carb,10));
//	int mealid=Natives.mealsNr();
	if(carb==0.0f)
		repeat.setVisibility(INVISIBLE);
	give.accept(carb,mealptr);
	float[] carbar={carb};
	int[] mealptrar={mealptr};
	roundlabel.setOnClickListener(v-> askround(act,()->{
		give.accept(carbar[0],mealptrar[0]);
		roundlabel.setText(act.getString(R.string.round)+Natives.getroundto());
		}));
		int width=GlucoseCurve.getwidth();
		int height=GlucoseCurve.getheight();

	Layout lay=new Layout(act,(l,w,h)-> {
		if(width>w) {
			if(numb.noroom)
				l.setX(width-w);
			else {
				int half= width/2;
				int bij=(half-w)/4;
				l.setX(half+bij);
				}
			   }

			if(h>=0.9f*height)
				l.setY(0);
			else
				l.setY((height-h)/2);

		return new int[]{w,h};
		},new View[]{roundlabel,repeat},new View[]{recycle},new View[] {add,Help,close});
    	lay.setBackgroundColor(Applic.backgroundcolor);
	act.addContentView(lay, new ViewGroup.LayoutParams(width/2, height));
	repeat.setOnClickListener(v->{
	     removeContentView(lay);
	     act.hideSystemUI();
	     if(mealptrar[0]!=0) {
		   if(numb.currentnum!=0&&numb.currentnum!= numio.newhit) {
		        Log.i(LOG_ID,"repeat");
			Natives.hitsetmealptr(numb.currentnum,mealptrar[0]);
			}	
		    int ptrcp=Natives.cpmeal(mealptrar[0]);
		    act.poponback();
		    numb.addnumberwithmenu(act,ptrcp);
		}
	});
	close.setOnClickListener(v-> act.doonback());
	act.setonback(() -> {
		lay.setVisibility(GONE);
		removeContentView(lay);
		act.hideSystemUI();
//		give.accept(mealptrar[0],carbar[0]);
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
		numb.showkeyboard(act);
		menuitem(act,numb,mealptrar[0],i,onsave,carbar[0]);
		};
	consar.cons=hiercons;
	recycle.setAdapter(foodadapt);

	add.setOnClickListener(v-> {
		lay.setVisibility(GONE);
		numb.showkeyboard(act);
		menuitem(act,numb,mealptrar[0],-1,onsave,carbar[0]);
		});
	
	return lay;
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
	EditText total=geteditview(act,focushere);
	total.setMinEms(5);
	TextView mealtotallabel=getlabel(act,R.string.mealtotal);
	EditText mealtotal=geteditview(act,focushere);
	mealtotal.setMinEms(5);
	TextView amountlabel=getlabel(act,R.string.quantity);
	EditText amount	= geteditview(act,focushere);

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
		int width=GlucoseCurve.getwidth();
		int hei=GlucoseCurve.getheight();
		if(width>w)
			l.setX(width/2-w);
		if(hei>h)
			l.setY((hei-h)/2);
		return new int[]{w,h};
		},new View[]{amountlabel,amount},new View[]{ingrlabel,Ingredient},new View[]{carblabel,carbos},new View[]{totallabel,total}, new View[]{mealtotallabel,mealtotal},
			new View[] {Delete,Cancel,Save});
	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
	   lay.setPadding(pad,0,pad,0);
	act.addContentView(lay, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));
//	      lay.setBackgroundResource(R.drawable.dialogbackground);
    	lay.setBackgroundColor(Applic.backgroundcolor);

	Delete.setOnClickListener(v-> {
		int newmealptr=Natives.deletefrommeal(mealptr,pos);
		give.accept(newmealptr);
		removeContentView(lay);
	        numb.hidekeyboard();
		act.hideSystemUI();
		act.poponback();
		});
	Cancel.setOnClickListener(v-> act.doonback());
	 act.setonback(() -> {
		lay.setVisibility(GONE);
		give.accept(-1);
		removeContentView(lay);
	        numb.hidekeyboard();
		act.hideSystemUI();
	 	});
	Ingredient.setOnClickListener(v-> selectingredient(act, i->{
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
		}));
	Save.setOnClickListener(v-> {
		if(ingred[0]==-1) {
			Toast.makeText(act, R.string.specifyingredient, Toast.LENGTH_SHORT).show();
			return;
			}
		 float am=str2float( amount.getText().toString());
		 if(am==0.0f) {
			Toast.makeText(act, R.string.specifyhowmuch, Toast.LENGTH_SHORT).show();
			return;
		 	}	
		int newmealptr=Natives.changemealitem(mealptr,pos,ingred[0],am);
		lay.setVisibility(GONE);
		removeContentView(lay);
	        numb.hidekeyboard();
		act.hideSystemUI();
		give.accept(newmealptr);
		act.poponback();
		});
	}

static public class IngredientViewAdapter extends RecyclerView.Adapter<IngredientViewHolder> {
	Consptr ingrindex;
    IngredientViewAdapter(Consptr ingrindex) {
    	this.ingrindex=ingrindex;

	    }

    @NonNull
	@Override
    public IngredientViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
    	Button view=new Button( parent.getContext());

	view.setTransformationMethod(null);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f);
	ViewGroup.LayoutParams params= new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
	view.setLayoutParams(params);
       	view.setGravity(Gravity.CENTER_VERTICAL | Gravity.LEFT);
        return new IngredientViewHolder(view,ingrindex);

    }

	@Override
	public void onBindViewHolder(final IngredientViewHolder holder, int pos) {
		TextView text=(TextView)holder.itemView;
		text.setText(Natives.ingredientName(pos));
	    }
        @Override
        public int getItemCount() {
                return Natives.ingredientNr();
        	}

}

static void selectingredient(MainActivity act,IntConsumer setindex) {
	RecyclerView recycle = new RecyclerView(act);
	LinearLayoutManager lin = new LinearLayoutManager(act);
	recycle.setLayoutManager(lin);
	var add=getbutton(act,R.string.define);
	Button close=getbutton(act,R.string.closename);
	CheckBox edit=new CheckBox(act);
	edit.setText(R.string.edit);
	recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, MATCH_PARENT));
	int height=GlucoseCurve.getheight();
	int width=GlucoseCurve.getwidth();
	int viewwidth=(int)(width*.56);
	int xpos=(width-viewwidth)/2;
	int ypos= 0;
	Layout lay=new Layout(act,(l,w,h)-> {

//		l.setY((height-h)/2);
		return new int[]{w,h};
		},new View[]{recycle},new View[] {add,edit,close});
	lay.setX(xpos);
	lay.setY(ypos);
	IntConsumer hiercons=i-> {
		lay.setVisibility(GONE);
		removeContentView(lay);
		act.poponback();
		setindex.accept(i);
		};
	Consptr consar=new Consptr(hiercons);
	IngredientViewAdapter foodadapt = new IngredientViewAdapter(consar); //USE
	recycle.setAdapter(foodadapt);

	edit.setOnCheckedChangeListener((buttonView, isChecked)-> {
		if(isChecked) {
			consar.cons=i-> {
				edit.setChecked(false);
				defineingredient(act,foodadapt,recycle,i,setindex);
				};
			}
		else
			consar.cons=hiercons;
		});


        lay.setBackgroundResource(R.drawable.dialogbackground);
	act.addContentView(lay, new ViewGroup.LayoutParams(viewwidth, height));
	lay.invalidate();
	lay.setVisibility(VISIBLE);
	lay.bringToFront();
	close.setOnClickListener(v-> act.doonback());
	act.setonback(() -> {
		lay.setVisibility(GONE);
		removeContentView(lay);
		act.hideSystemUI();
		});
	add.setOnClickListener(v-> defineingredient(act,foodadapt,recycle,-1,setindex));
	
	}
static void	defineingredient(MainActivity act ,IngredientViewAdapter  foodadapt,RecyclerView recycle,int pos, IntConsumer  setindex) {
	act.showSystemUI();
	act.showui=true;
	TextView.OnEditorActionListener	 actlist= new TextView.OnEditorActionListener() {
		    @Override
		    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
		    if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
				    || actionId == EditorInfo.IME_ACTION_DONE) {
		//		act.hideSystemUI();
				 tk.glucodata.help.hidekeyboard(act);
				 Log.i(LOG_ID,"onEditorAction");
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
        unit.setMinEms(6);
	TextView carblabel=getlabel(act,R.string.carbperunit);
	EditText carb=new EditText(act);
	carb.setMinEms(5);
	carb.setInputType(InputType.TYPE_CLASS_NUMBER |InputType.TYPE_NUMBER_FLAG_DECIMAL);//| InputType.IME_FLAG_NO_FULLSCREEN);
//	carb.setKeyListener(DigitsKeyListener.getInstance("0123456789^*/+-()."));
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
	//	spinner.clearAnimation();
		spinner.setSelection(usedunit);
		}
	else
		spinner.setVisibility(GONE);
	Layout lay=new Layout(act,(l,w,h)-> {
//		int height=GlucoseCurve.getheight();
		int width=GlucoseCurve.getwidth();
		if(width>w)
			l.setX((width-w)/2);
		l.setY(0);
		return new int[]{w,h};
		},new View[]{namelabel,name},new View[] {unitlabel,unit,spinner},new View[]{carblabel,carb},new View[] {Cancel,Database,Delete,Save});

	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
	   lay.setPadding(pad,0,pad,pad/2);
	  lay.setBackgroundResource(R.drawable.dialogbackground);
	act.addContentView(lay, new ViewGroup.LayoutParams(WRAP_CONTENT, WRAP_CONTENT));

	name.requestFocus();
	Save.setOnClickListener(v-> {
		final float flcarb=edit2float(carb);
		Natives.saveingredient(pos,name.getText().toString(), unit.getText().toString(),flcarb);

		foodadapt.notifyDataSetChanged();
		lay.setVisibility(GONE);
		removeContentView(lay);
		act.showui=false;
		act.hideSystemUI();
		act.poponback();
		if(pos<0)
			recycle.scrollToPosition(foodadapt.getItemCount()-1);
		});
	Runnable endproc= ()-> {
		lay.setVisibility(GONE);
		removeContentView(lay);
		act.showui=false;
		act.hideSystemUI();
		};
	Cancel.setOnClickListener(v-> {
		endproc.run();
		act.poponback();
		});
	
	act.setonback(endproc);
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
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18f);
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
	RecyclerView recycle = new RecyclerView(act);
	LinearLayoutManager lin = new LinearLayoutManager(act);
	recycle.setLayoutManager(lin);
	long[] hitptr	={0L};
	MealDatabaseViewAdapter foodadapt = new MealDatabaseViewAdapter(hitptr,give); //USE
	recycle.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
	recycle.setAdapter(foodadapt);
	Button searchbutton=getbutton(act,R.string.search);
	EditText searchstr= new EditText(act);
        searchstr.setInputType(InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
	int searchoptions=EditorInfo.IME_FLAG_NO_EXTRACT_UI| EditorInfo.IME_FLAG_NO_FULLSCREEN| EditorInfo.IME_ACTION_SEARCH;
        searchstr.setImeOptions(searchoptions);
//        searchstr.setMinEms(15);
	searchstr.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
	searchstr.setOnEditorActionListener(new TextView.OnEditorActionListener() {
	    @Override
	    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
	    if (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
                            || actionId == EditorInfo.IME_ACTION_SEARCH) {
			hidekeyboard(act);
	//		act.hideSystemUI();
			hitptr[0]=Natives.foodsearch(searchstr.getText().toString());
			foodadapt.notifyDataSetChanged();
                        return true;
		   }
	    return false;
	    }
	});


	Button Close=getbutton(act,R.string.closename);
	Button Help=getbutton(act,R.string.helpname);
//	act.hideSystemUI();
	int fnr=Natives.foodnr();
	recycle.scrollToPosition(random.nextInt(fnr));
	Layout lay=new Layout(act,(l,w,h)->{
		int width=GlucoseCurve.getwidth();
		l.setY(0);
		if(width>w)
			l.setX((width-w)/2);
		return new int[]{w,h};},new View[]{Help,searchstr,searchbutton,Close,}	,new View[]{recycle});
	   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
	   lay.setPadding(pad,0,pad,0);
    	lay.setBackgroundColor(Applic.backgroundcolor);
	act.addContentView(lay, new ViewGroup.LayoutParams(MATCH_PARENT, MATCH_PARENT));
	searchstr.requestFocus();
	Help.setOnClickListener(v-> { 
//		act.hideSystemUI();
//		help.help(R.string.searchhelp,act);
		help.help(R.string.nutrients,act);
		});
	Close.setOnClickListener(v-> { 
		act.doonback();
		});
	act.setonback(() -> { 
		removeContentView(lay);
	//	act.hideSystemUI();
		Natives.freefoodptr(hitptr[0]);
		});
	searchbutton.setOnClickListener(v-> { 
		hidekeyboard(act);
	//	act.hideSystemUI();
//		Natives.freefoodptr(hitptr[0]);
		hitptr[0]=Natives.foodsearch(searchstr.getText().toString());
		foodadapt.notifyDataSetChanged();
		});
	return lay;
	}
static String[] compnames=Natives.getcomponentlabels( );
static String[] compunits=Natives.getcomponentunits( );
static void	shownutrients(MainActivity act,int id,boolean showzero,TriConsumer<String,Float,String> give) {
	hidekeyboard(act);
//	act.hideSystemUI();
	TextView ingred=new TextView(act);
	final String label= Natives.idfoodlabel(id);
	ingred.setText(label);
        ingred.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24f);
	int[] ingr=Natives.getcomponents(id);
	int rows=2;
	for(int el:ingr) {
		if((showzero&&el!=-1)||el>0)
			rows++;
		}
	measuredgrid grid=new measuredgrid(act);
        int cols=4;
        grid.setColumnCount(cols);
        grid.setRowCount(rows);
	GridLayout.LayoutParams params = new GridLayout.LayoutParams();
	params.columnSpec = GridLayout.spec(0, 4); 
//	ingr.setLayoutParams(params);
	grid.addView(ingred,params);

	GridLayout.LayoutParams para = new GridLayout.LayoutParams();
	para.columnSpec = GridLayout.spec(1, 3); 
	TextView per100=getlabel(act,R.string.compositionof100gram);
	grid.addView(per100,para);
   int pad=(int)(tk.glucodata.GlucoseCurve.metrics.density*5.0);
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
        CheckBox zero=new CheckBox(act);
        zero.setText(R.string.showzero);
        zero.setChecked(showzero);
	   zero.setPadding(0,0,0,0);
	GridLayout.LayoutParams parzero = new GridLayout.LayoutParams();
 int padzero=(int)	(tk.glucodata.GlucoseCurve.metrics.density*7.0);
	parzero.setMargins(padzero, 0, padzero, 0);
	ScrollView scroll=new ScrollView(act);
	
    grid.setmeasure((l,w,h)-> {
		int height=GlucoseCurve.getheight();
		int width=GlucoseCurve.getwidth();
		if(width>w)
			scroll.setX((width-w)/2);
		if(height>h)
			scroll.setY((height-h)/2);
    		});
	Button Select=getbutton(act,R.string.select);
	grid.addView(Select);
	zero.setOnCheckedChangeListener((buttonView, isChecked)-> {
		act.doonback();
//		act.hideSystemUI();
		shownutrients(act,id, isChecked,give) ;
		return;
		});
	grid.addView(zero,parzero);
	Select.setOnClickListener(v-> {
		removeContentView(scroll);
//		act.hideSystemUI();
		act.poponback();
		act.poponback();
		give.accept(label,ingr[0]/100000.0f,compunits[0]);
		});
	Button Close=getbutton(act,R.string.closename);
	GridLayout.LayoutParams closeparams = new GridLayout.LayoutParams();
	closeparams.columnSpec = GridLayout.spec(2, 2);
	grid.addView(Close,closeparams);
//	act.addContentView(grid,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
//	scroll.setFillViewport(true);
	   grid.setPadding(pad,0,0,0);
	scroll.addView(grid);
	scroll.setSmoothScrollingEnabled(false);
        scroll.setVerticalScrollBarEnabled(false);
	act.addContentView(scroll,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        scroll.setBackgroundResource(R.drawable.dialogbackground);
	Close.setOnClickListener(v-> act.doonback());
	act.setonback(() -> {
		removeContentView(scroll);
//		act.hideSystemUI();
		});

}
}
