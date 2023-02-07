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

/*kind of ArrayAdapter when you don't want to show the last item */

import android.content.Context;
import android.database.DataSetObserver;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SpinnerAdapter;
import android.widget.TextView;

import java.util.AbstractList;

import static android.graphics.Color.BLUE;
import static android.graphics.Color.RED;
import static tk.glucodata.Applic.isWearable;

public class LabelAdapter<T> implements SpinnerAdapter {
    private final LayoutInflater mInflater;
//private Activity act;
private static final String LOG_ID="LabelAdapter";
    private AbstractList<T> ar;
    private final int eraf;
public void setarray(AbstractList<T> inar) {
	ar=inar;
	}
public AbstractList<T>  getarray() {
	return ar;
	}
    public LabelAdapter(Context context, AbstractList<T> ar,final int eraf) {
    	////Log.i(LOG_ID,"LabelAdapter");
//	act=context;
        this.ar=ar;
	this.eraf=eraf;
        mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);

    }

    @Override
    public View getDropDownView(int position, View convertView, ViewGroup parent) {
      //Log.i(LOG_ID,"getDropDownView "+position);
        if (convertView == null) {
            convertView = mInflater.inflate(android.R.layout.simple_spinner_dropdown_item, parent, false);
        }
	
	TextView text= ((TextView)(convertView.findViewById(android.R.id.text1)));
	if(position<ar.size()) {
		T str=ar.get(position);
		text.setText(str.toString());
		}
	else
		text.setText("Error!!");
        if(isWearable) {
            var width=GlucoseCurve.getwidth();
            convertView.setX(width/4);
            }
        return convertView;

    }

    @Override
    public void registerDataSetObserver(DataSetObserver observer) {

    }

    @Override
    public void unregisterDataSetObserver(DataSetObserver observer) {

    }

    @Override
    public int getCount() {
        return ar.size()-eraf;
    }

    @Override
    public Object getItem(int position) {
        return position;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public boolean hasStableIds() {
        return true;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
       //Log.i(LOG_ID,"getView "+position);
           if (convertView == null) {
                convertView = mInflater.inflate(android.R.layout.simple_spinner_item, parent, false);
            }
	var text=((TextView)(convertView.findViewById(android.R.id.text1)));
	if(position<ar.size()) {
		T str=ar.get(position);
		text.setText(str.toString());
		}
	else {
		text.setText("Error!!");
		}
//	text.setBackgroundColor(RED);
        return convertView;
        }



    @Override
    public int getItemViewType(int position) {
        return 0;
    }

    @Override
    public int getViewTypeCount() {
        return 1;
    }

    @Override
    public boolean isEmpty() {
        return ar.size()==eraf;
    }
}
