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
import android.os.Bundle;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.CompoundButton;
import android.widget.TextView;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.Log.doLog;

public class Layout extends ViewGroup {
/*
static View.AccessibilityDelegate  accessDeli=new View.AccessibilityDelegate () {
        @Override
        public void onInitializeAccessibilityNodeInfo( View host, AccessibilityNodeInfo info) {
            String message=(host  instanceof TextView)? ((TextView)host).getText().toString() :host.toString();

            if(host.isEnabled()&&host.getVisibility()==View.VISIBLE) {
                {if(doLog) {Log.i(LOG_ID,"SHOW onInitializeAccessibilityNodeInfo "+message);};};
                super.onInitializeAccessibilityNodeInfo(host, info);
                }
            else {
                {if(doLog) {Log.i(LOG_ID,"HIDE onInitializeAccessibilityNodeInfo "+message);};};
                }
        }



    };

static View.AccessibilityDelegate accessDeli = new View.AccessibilityDelegate() {
    @Override
    public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(host, info);
        boolean showinfo=host.isEnabled() && host.getVisibility() == View.VISIBLE;

        if (doLog)      {
                String message = (host instanceof TextView)
                        ? ((TextView) host).getText().toString()
                        : host.toString();
            Log.i(LOG_ID, (showinfo ? "SHOW" : "HIDE") + " onInitializeAccessibilityNodeInfo " + message);
                }

        if (!showinfo) {
            info.setVisibleToUser(false);
            info.setContentDescription(null);
        }
    }
}; */
static final View.AccessibilityDelegate accessDeli = new View.AccessibilityDelegate() {
  @Override
  public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {
    boolean showinfo = host.isEnabled() && host.getVisibility() == View.VISIBLE;

    try {
      super.onInitializeAccessibilityNodeInfo(host, info);
    } catch (IllegalArgumentException e) {
      // Workaround for buggy frameworks/services producing invalid checked state.
      Log.i(LOG_ID,"accessDeli class="+host.getClass().getName()+" id="+ host.getId());
      info.setClassName(host.getClass().getName());
      info.setEnabled(host.isEnabled());
      info.setVisibleToUser(host.getVisibility() == View.VISIBLE);

      if (host instanceof TextView) info.setText(((TextView) host).getText());

      if (host instanceof CompoundButton cb) {
        info.setCheckable(true);
        info.setChecked(cb.isChecked()); // boolean path
      }
    }

    if (!showinfo) {
      info.setVisibleToUser(false);
      info.setContentDescription(null);
    }
  }
};
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
static int[]  noneplacer(View l,int w,int h) {
     return new int[] {w,h};
    };

void init(Context context,Placer placer,int nr) {
//    setLayoutDirection(LAYOUT_DIRECTION_LTR);
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
public Layout(Context context, Object [] ... rows) {
    this(context,true,Layout::noneplacer,rows);
    }
public Layout(Context context, boolean rev, Object [] ... rows) {
    this(context,rev,Layout::noneplacer,rows);
    }

 private static Object[] arrayReverse(Object[] array) {
     int len=array.length;
     Object[] revview=new Object[len];
     for(int i=0,uit=len-1;i<len;) {
       revview[uit--] = array[i++];
       }
    return revview;
   }

private static Object[][] reverseAll(Object[][] views) {
    int len= views.length;
    var revviews=new Object[len][];
    for(int i=0;i<len;++i) {
        if(views[i]!=null) 
            revviews[i]=arrayReverse(views[i]);
        }
    return revviews;
    }


private void addRowChildren(Object[] rowobjects,int row) {
          if(rowobjects!=null) {
            for(Object obel:rowobjects) {
                if(obel!=null) {
                    if(obel instanceof View) {
                        addEl((View) obel,row);
                        }
                    else {
                        if(obel instanceof View[]) {
                            for(var v: (View[])obel) {
                                if(v!=null)
                                    addEl(v,row);
                                }
                            }
                        }
                 }
               }
             }
       rowend[row]=getChildCount();
       }
private void addEl(View el,int row) {
        el.setAccessibilityDelegate(accessDeli);
        addView(el);
        el.setTag(R.id.layoutrow,row);
        }
public Layout(Context context,Placer placer, Object [] ... inrows) {
    this(context,true,placer,inrows);
    }
public Layout(Context context,boolean rev,Placer placer, Object [] ... inrows) {
        super(context);
        Object[][] rows= (rev&&MainActivity.rtl)?reverseAll(inrows):inrows;
        rownr= rows.length;
        init(context,placer,rownr);
        for(int i=0;i<rownr;i++) {
          addRowChildren(rows[i],i);
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
//    System.arraycopy(rowend,index+1,rowend,index,rownr-index);
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
public int addrow(boolean rev,Object[] inrow) {
   final Object[]  row=(rev&&MainActivity.rtl)?arrayReverse(inrow):inrow;
   final int prevnr=rownr;
    if(rowend.length==rownr++) { 
        int[] oldrowend =rowend;
        reserve(rownr);
        System.arraycopy(oldrowend,0,rowend,0,prevnr);
        }

    addRowChildren(row,prevnr);
  
    return rownr;
    }
public int addrow(Object[] inrow) {
    return addrow(true,inrow);
    }
static private int childWidth(View child) {
      return  Math.max(child.getMinimumWidth(),child.getMeasuredWidth());
   }
static private int childHeight(View child) {
      return  Math.max(child.getMinimumHeight(),child.getMeasuredHeight());
   }

int rowgeo(final int start,final int row,int widthMeasureSpec, int heightMeasureSpec) {
   int end=rowend[row];
   int maxWidth=0,totHeight=0;
   int maxbaseline=0;
   int not=0;
   matchparent[row]=null;
   for(int c=start;c<end;c++) {
       View child = getChildAt(c);
        if(child!=null&&child.getVisibility()!=GONE) {
          ViewGroup.LayoutParams  params=child.getLayoutParams();
          int leftmargin,rightmargin,topmargin,bottommargin;
          if(params instanceof ViewGroup.MarginLayoutParams) {
            var margins=(ViewGroup.MarginLayoutParams)params;
            leftmargin=margins.leftMargin;
            rightmargin=margins.rightMargin;
            topmargin=margins.topMargin;
            bottommargin=margins.bottomMargin;
//            if(child instanceof TextView) Log.i(LOG_ID,"rowgeo c="+c+" "+((TextView)child).getText()+" leftmargin="+leftmargin);
            }
          else {
            leftmargin=rightmargin=bottommargin=topmargin=0;
            }
          if(params!=null&&params.width==MATCH_PARENT) {
               matchparent[row]=child;
               measureChild(child,1, heightMeasureSpec);
               maxWidth+=child.getMinimumWidth()+leftmargin+rightmargin;

               }
          else {
                measureChild(child,widthMeasureSpec, heightMeasureSpec);
                maxWidth +=  childWidth(child)+leftmargin+rightmargin;
                }
          not++;
          final int h = childHeight(child)+topmargin+bottommargin;
          if (totHeight < h) totHeight = h;
          if(usebaseline) {
              int baseline=child.getBaseline();
              if(baseline<0) baseline=(int)(h/2-basefromiddle);
              if(baseline>maxbaseline) maxbaseline=baseline;
              }
         }
   }
    notgone[row]=not;
    maxwidths[row]=maxWidth;
    baselines[row]=maxbaseline;
    return totHeight;
    }
boolean usebaseline=true;
int rowmax;
int totHeight;
int maxHeight; 


private int[] domeasure(int widthMeasureSpec, int heightMeasureSpec) {
   totHeight =  getPaddingTop() + getPaddingBottom();
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
    for(int el:maxwidths) {
        if(el>maxWidth)
            maxWidth=el;
      }
   maxWidth+=getPaddingLeft() + getPaddingRight();
    maxWidth = Math.max(maxWidth, getSuggestedMinimumWidth());
   int    prevrw = resolveSizeAndState(maxWidth, widthMeasureSpec, 0);

    if(totHeight< getSuggestedMinimumHeight())
        totHeight = getSuggestedMinimumHeight();
     int prevrh=resolveSizeAndState(totHeight, heightMeasureSpec, 0);
    int[] make=placer.place(this,prevrw,prevrh);
    if(make!=null&&make.length==2) {
        //Log.i(LOG_ID,"domeasure make[0]="+make[0]+ " make[1]="+make[1]);
        return make;
        }
     else  {
        //Log.i(LOG_ID,"domeasure prevrh="+prevrh+ " prevrw="+prevrw);
        return  new int[] {prevrw,prevrh};
        }
    }

boolean useMatch=false;
@Override
protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
   //Log.i(LOG_ID,"onMeasure "+widthMeasureSpec);
   int[] res=domeasure(widthMeasureSpec, heightMeasureSpec);
 
    if(useMatch) {
       boolean hasmatch=false;
       int usewidth=res[0]-(getPaddingLeft() + getPaddingRight());

       for(int i=0;i<rownr;i++) {
             var child=matchparent[i];
             if(child!=null) {
                 var  params=child.getLayoutParams();
                 if(params instanceof ViewGroup.MarginLayoutParams) {
                    var margins=(ViewGroup.MarginLayoutParams)params;
                    params.width=usewidth-(margins.leftMargin+margins.rightMargin);
                    }
                 else
                     params.width=usewidth;
                 child.setLayoutParams(params);
                hasmatch=true;
                }
            }
        if(hasmatch) {
            res=domeasure( widthMeasureSpec,  heightMeasureSpec);
            }  
        }
    //Log.i(LOG_ID,"onMeasure prevrh="+res[0]+ " prevrw="+res[1]);
    setMeasuredDimension(res[0],res[1]);
    }
/*
    Als het aantal in rij 1 is,
        begin met (width-widthchild)/2
   als groter dan 1;
   leftmarg=0;
   tussen= (width-sumwidth)/ (end-start-1)


    */
/*public boolean round=false;
int getwidth(int topin,int maxheight) {
   final var width=getWidth();
   if(!round)
      return width;
   var top=topin+maxheight*.3; 
   {if(doLog) {Log.i(LOG_ID,"y="+top);};};
   var half=width*.5;
   if(top>=half)
      return width;
  var left=half-top;
  return (int)(Math.sqrt(Math.pow(half,2)-Math.pow(left,2))*2);
   }
int getleft(int hierwidth) {
   if(!round)
      getPaddingLeft();
   var width=getWidth();
   return (int)((width-hierwidth)*.5+ getPaddingLeft());
   } */

final int layrow(final int top,final int start,final int row,final int maxheight) {
   int nr=notgone[row];
   if(nr==0) return top;
   var hierwidth=getWidth();
   var hierleft=getPaddingLeft();
   final int end=rowend[row];
   final int baseline=baselines[row];
   int maxwidth=hierwidth-getPaddingLeft()-getPaddingRight();
   if(nr==1) {
      View child=null;
      for(int i=start;i<end&&((child=getChildAt(i))==null||child.getVisibility()==GONE);i++) {
              };
      if(child==null)
        return top;
      ViewGroup.LayoutParams  params=child.getLayoutParams();
      int leftmargin,rightmargin,topmargin,bottommargin;
      if(params instanceof ViewGroup.MarginLayoutParams) {
        var margins=(ViewGroup.MarginLayoutParams)params;
        leftmargin=margins.leftMargin;
        rightmargin=margins.rightMargin;
        topmargin=margins.topMargin;
        bottommargin=margins.bottomMargin;
        }
      else {
        leftmargin=rightmargin=bottommargin=topmargin=0;
        }
      int width;
      int left;

      if(child==matchparent[row]) {
            width=maxwidth-leftmargin-rightmargin;
            left=hierleft+leftmargin;
            }
       else {
            width = childWidth(child);
            left=(maxwidth-width)/2+ getPaddingLeft()+leftmargin;
             }
       final int childtop=top+topmargin;
       final var childheight=childHeight(child);
       final var useheight=Math.min(childheight,maxheight);
      //  Log.i(LOG_ID,"child width="+width+" childwidth="+childWidth(child));
//       child.setMinimumHeight(useheight);
//        measureChild(child,width, useheight);
       child.layout(left, childtop, left + width, childtop+useheight);
       return top+Math.min(childheight+bottommargin+topmargin,maxheight);
       }
  int left =hierleft; 
  int tussen;
  if(matchparent[row]==null) {
      tussen=(maxwidth-maxwidths[row])/(nr-1);
      if(tussen<0)
         tussen=0;
      }
  else {
        tussen=0;
    }    
  int bottom=0;
  for(int i = start; i < end; i++) {
      View child = getChildAt(i);
      if(child!=null&&child.getVisibility()!=GONE) {
          ViewGroup.LayoutParams  params=child.getLayoutParams();
          int leftmargin,rightmargin,topmargin,bottommargin;
          if(params instanceof ViewGroup.MarginLayoutParams) {
            var margins=(ViewGroup.MarginLayoutParams)params;
            leftmargin=margins.leftMargin;
            rightmargin=margins.rightMargin;
            topmargin=margins.topMargin;
            bottommargin=margins.bottomMargin;
            }
          else {
            leftmargin=rightmargin=bottommargin=topmargin=0;
            }
         final int childheight= childHeight(child);
         int cheight= Math.min(childheight,maxheight-bottommargin-topmargin);
        int tophier;
         if(usebaseline) {
             int childbaseline=child.getBaseline();
             if(childbaseline<0) childbaseline=(int)(cheight/2-basefromiddle);
              tophier=(top+baseline-childbaseline)+topmargin;
              }
         else {
            tophier=top+topmargin;
            }
         int childwidth = childWidth(child);
         int width = child==matchparent[row]?(maxwidth-maxwidths[row]):childwidth;
         int childleft=left+leftmargin;
          int bot=tophier+cheight;
         child.layout(childleft, tophier, childleft + width, bot);
         bot+=bottommargin;
         if(bot>bottom) bottom=bot;
         left += (width+tussen)+leftmargin+rightmargin;
         }
       }
     return bottom;
    }
    @Override
protected void onLayout(boolean changed, int l, int t, int r, int b) {

    //Log.format("onLayout(,left=%d,top=%d,right=%d,bottom=%d) width=%d getWidth=%d\n",l,t,r,b,r-l,getWidth());
    int top=getPaddingTop();
    int ptop=t;
    int start=0;
    int height=b-t;
    int heightleft=height-totHeight;
    int yspace= (rownr>1&&heightleft>0)?(heightleft/(rownr-1)):0;
    for(int i=0;i<rownr;i++) {
        //Log.i(LOG_ID,"row="+i+" top="+top);
        if(i==rowmax) 
            top=layrow(top,start,i,maxHeight+heightleft);
        else {
            top=layrow(top,start,i,maxHeight);
            }
        top+=yspace;
        start=rowend[i];
        }
    }
public ViewGroup.MarginLayoutParams generateDefaultLayoutParams() {
   return new ViewGroup.MarginLayoutParams(WRAP_CONTENT, WRAP_CONTENT);
    }

/*
static class ScrollListener extends GestureDetector.SimpleOnGestureListener {
@Override
   public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
      {if(doLog) {Log.i(LOG_ID,"onScroll dX="+distanceX+" dY="+distanceY);};};
      return false;
      }
@Override
      public boolean onFling (MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
      {if(doLog) {Log.i(LOG_ID,"onFling volX="+velocityX+"volY="+velocityY);};};
      return false;
      }
};

*/
@Override
public boolean onTouchEvent(MotionEvent event) {
   return true;
   }
    @Override
    public boolean performClick() {
        return true; //Otherwise touch end up in underlying View
    } 

    /*@Override
    public boolean performClick() {
        super.performClick();
        return true; //Otherwise touch end up in underlying View
    } */

public static ViewGroup.MarginLayoutParams getMargins(View view) {
   final ViewGroup.LayoutParams  params=view.getLayoutParams();
   ViewGroup.MarginLayoutParams margins;
   if(params==null) 
          margins=new ViewGroup.MarginLayoutParams(WRAP_CONTENT,WRAP_CONTENT);
   else {
       if((params instanceof ViewGroup.MarginLayoutParams)) {
          return (ViewGroup.MarginLayoutParams)params;
          }
        margins=new ViewGroup.MarginLayoutParams(params);
        }
   view.setLayoutParams(margins);
   return margins;
   }

public static void addSystemMargins(View view) {
        if(view==null)
                return;
        var marg=getMargins(view);
        marg.topMargin= MainActivity.systembarTop;
        marg.bottomMargin= MainActivity.systembarBottom;
        marg.leftMargin= MainActivity.systembarLeft;
        marg.rightMargin= MainActivity.systembarRight;
        }
/*
 @Override
    public int getBaseline() {
        return 0;
    }  */

}
