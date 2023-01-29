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
/*      Fri Jan 27 15:31:05 CET 2023                                                 */


package tk.glucodata;

import android.content.Context;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;

public class Layout extends ViewGroup {
//    public Layout(Context context) { super(context); } public Layout(Context context, AttributeSet attrs) { super(context, attrs); } public Layout(Context context, AttributeSet attrs, int defStyle) { super(context, attrs, defStyle); } 
private static final String LOG_ID="Layout";

 private   void reserve(int nr) {
        rowend=new int[nr];
        notgone=new int[nr];
        baselines = new int[nr];
	maxwidths=new int[nr];
	matchparent=new View[nr];
	}
	View[] matchparent;
    float basefromiddle;
    int[] maxwidths=null;
  private int[] rowend =null;
  private int[] notgone =null;
   private int rownr=0;
   private  int[] baselines=null;
  private tk.glucodata.Placer placer;
static int[]  noneplacer(Layout l,int w,int h) {
	 return new int[] {w,h};
	};
void init(Context context,Placer placer,int nr) {
        this.placer=placer;
	reserve(nr);
     	TextView prob=new TextView(context);
	Paint.FontMetrics met= prob.getPaint().getFontMetrics();
	basefromiddle=met.ascent/2.0f;
	}
    public Layout(Context context,Placer placer, int nr) {
        super(context);
	init(context,placer,nr);
    	}
    public Layout(Context context, View [] ... rows) {
	this(context,Layout::noneplacer,rows);
	}
    public Layout(Context context,Placer placer, View [] ... rows) {
        super(context);
        rownr= rows.length;
	init(context,placer,rownr);
        for(int i=0;i<rownr;i++) {
	  if(rows[i]!=null) {
	    for(View el:rows[i]) {
		if(el!=null) {
		    addView(el);
		    el.setTag(R.id.layoutrow,i);
		    }
		 }
	       }
            rowend[i]=getChildCount();
             }

    }
    public void empty() {
    	rownr=0;
	removeAllViews();
    	}
public void delrow(int index) {
	if(index>=rownr)
		return;
	int start=(index==0)?0:rowend[index-1],end=rowend[index]; 
	for(int i=end-1;i>=start;i--)
		removeViewAt(i);
	rownr--;
	int len=end-start;
	for(int to=index;to<rownr;to++) {
		rowend[to]=rowend[to+1]-len;
		for(int i=start;i<rowend[to];i++) {
		 	View view=getChildAt(i);
			view.setTag(R.id.layoutrow,to);
		 	}
		start=rowend[to];
		}
//	System.arraycopy(rowend,index+1,rowend,index,rownr-index);
	}
public int getviewrow(View v) {
	return (Integer)v.getTag(R.id.layoutrow);
	}


public View[] getrow(int index) {
	if(index>=rownr)
		return null;
	int start=(index==0)?0:rowend[index-1],end=rowend[index]; 
	int len=end-start;
	View[] views=new View[len];
	for(int i=0;i<len;i++) {
		views[i]=getChildAt(start+i);
		}
	return views;
	}
public int addrow(View [] row) {
   final int prevnr=rownr;
	if(rowend.length==rownr++) { 
		int[] oldrowend =rowend;
		reserve(rownr);
		System.arraycopy(oldrowend,0,rowend,0,prevnr);
		}
	for(View el:row) {
		if(el!=null) {
		    addView(el);
		    el.setTag(R.id.layoutrow,prevnr);
		    }
	      }
        rowend[prevnr]=getChildCount();
	return rownr;
	}
    int rowgeo(final int start,final int row,int widthMeasureSpec, int heightMeasureSpec) {
	int end=rowend[row];
        int maxWidth=0,totHeight=0;
        int maxbaseline=0;
	int not=0;
	matchparent[row]=null;
        for(int c=start;c<end;c++) {
            View child = getChildAt(c);
	    if(child.getVisibility()!=GONE) {
		ViewGroup.LayoutParams  params ;
		if((params=child.getLayoutParams())!=null&&params.width==MATCH_PARENT) {
			matchparent[row]=child;
		 	measureChild(child,1, heightMeasureSpec);
			}
		else {
		     measureChild(child,widthMeasureSpec, heightMeasureSpec);
		    maxWidth += child.getMeasuredWidth();
		    }
	    not++;
	    final int h = child.getMeasuredHeight();
	    if (totHeight < h) totHeight = h;
	    // Log.i(LOG_ID,"rowgeo "+row+" h="+h+" rowHeight="+totHeight);
	    int baseline=child.getBaseline();
	    if(baseline<0)
		baseline=(int)(h/2-basefromiddle);

	    if(baseline>maxbaseline)
		maxbaseline=baseline;
		}
            }
	notgone[row]=not;
	maxwidths[row]=maxWidth;
	baselines[row]=maxbaseline;
        return totHeight;
    }
int rowmax;
int totHeight;
int maxHeight; 
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
       
//        measureChildren(WRAP_CONTENT, WRAP_CONTENT);
//        measureChildren(widthMeasureSpec, heightMeasureSpec);


        totHeight =  getPaddingTop() + getPaddingBottom();
	// Log.i(LOG_ID,"heightpadding="+totHeight);
	maxHeight=0; 
	rowmax=-1;
        for(int i=0,start=0;i<rownr;i++) {
		int height= rowgeo(start, i,widthMeasureSpec, heightMeasureSpec);
		if(height>maxHeight) {
			maxHeight=height;
			rowmax=i;
			}
		totHeight += height;
		start=rowend[i];
		}

        int maxWidth = 0;
	for(int el:maxwidths)
		if(el>maxWidth)
			maxWidth=el;
        maxWidth+=                getPaddingLeft() + getPaddingRight();
	if(totHeight< getSuggestedMinimumHeight())
		totHeight = getSuggestedMinimumHeight();
        maxWidth = Math.max(maxWidth, getSuggestedMinimumWidth());
	// Log.i(LOG_ID,"maxWidth="+maxWidth);
        int    prevrw = resolveSizeAndState(maxWidth, widthMeasureSpec, 0);
	// Log.i(LOG_ID,"Width="+prevrw);

	 int prevrh=resolveSizeAndState(totHeight, heightMeasureSpec, 0);
	 // Log.i(LOG_ID,"totHeight="+totHeight+" prevrh="+prevrh);
        int[] make=placer.place(this,prevrw,prevrh);
	if(make!=null&&make.length==2)
		setMeasuredDimension(make[0],make[1]);
	else
		setMeasuredDimension(prevrw,prevrh);
    }
/*
    Als het aantal in rij 1 is,
    	begin met (width-widthchild)/2
   als groter dan 1;
   leftmarg=0;
   tussen= (width-sumwidth)/ (end-start-1)


    */
    final int layrow(final int top,final int start,final int row,final int maxheight) {
	final int end=rowend[row];
	final int baseline=baselines[row];
       int maxwidth=getWidth()-getPaddingLeft()-getPaddingRight();
    	int nr=notgone[row];
	if(nr==0) 
		return top;
	if(nr==1) {
                View child=null;
		for(int i=start;i<end&&(child=getChildAt(i)).getVisibility()==GONE;i++) {
			};
		int height= Math.min(child.getMeasuredHeight(),maxheight);
		int bottom=top+height;
		int width;
		int left;
//	        ViewGroup.LayoutParams  params ; if((params=child.getLayoutParams())!=null&&params.width==MATCH_PARENT) {
	        if(child==matchparent[row]) {
			left=getPaddingLeft();
			width=maxwidth;
			}
		else {
			width = child.getMeasuredWidth();
			left=(maxwidth-width)/2+ getPaddingLeft();
			}
		child.layout(left, top, left + width, bottom);
		return bottom;
		}
        int left = getPaddingLeft();
		
	int tussen;
	if(matchparent[row]==null)
		tussen=(maxwidth-maxwidths[row])/(nr-1);
	else {
		tussen=0;
		}	
        int bottom=0;
        for(int i = start; i < end; i++) {
            View child = getChildAt(i);
	    if(child.getVisibility()!=GONE) {
		int childbaseline=child.getBaseline();
	//	int height= child.getMeasuredHeight();
		final int cheight= child.getMeasuredHeight();
		int height= Math.min(cheight,maxheight);
		if(childbaseline<0) childbaseline=(int)(cheight/2-basefromiddle);
		int tophier=(top+baseline-childbaseline);
//		int width = child.getMeasuredWidth(); //TODO set next
		int width = child==matchparent[row]?(maxwidth-maxwidths[row]):child.getMeasuredWidth();
//		int width = (child==matchparent[row]?(maxwidth-maxwidths[row]):0)+child.getMeasuredWidth();
		int bot= tophier + height;
		child.layout(left, tophier, left + width, bot);
		if(bot>bottom) bottom=bot;
		left += (width+tussen);
		}
           }
	// // Log.i(LOG_ID,row+" width="+(left-tussen));
        return bottom;
    }
    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
    	// Log.format("onLayout(,%d,%d,%d,%d) width=%d getWidth=%d\n",l,t,r,b,r-l,getWidth());
        int top=getPaddingTop()+t;
//        int start=getPaddingLeft()+l;
        int start=0;
	int height=b-t;
	int heightleft=height-totHeight;
	int yspace= (rownr>1&&heightleft>0)?(heightleft/(rownr-1)):0;
        for(int i=0;i<rownr;i++) {
	    if(i==rowmax) 
		    top=layrow(top,start,i,maxHeight+heightleft);
	    else {
		    top=layrow(top,start,i,maxHeight);
		    }
	  
	    top+=yspace;
            start=rowend[i];
        }
    }
public ViewGroup.LayoutParams generateDefaultLayoutParams() {
	return super.generateDefaultLayoutParams();
	}
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return true;
    }
	/*@Override
	public boolean performClick() {
    	super.performClick();
		return true; //Otherwise touch end up in underlying View
	} */
}
