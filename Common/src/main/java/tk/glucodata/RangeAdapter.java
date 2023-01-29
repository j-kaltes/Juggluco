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
import android.database.DataSetObserver;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SpinnerAdapter;
import android.widget.TextView;

import java.util.List;


public class RangeAdapter<T> implements SpinnerAdapter {
private final String LOG_ID="RangeAdapter";
//Function<T,R> R apply(T t)
/*
    public interface Displayer<T> {
        String displayer(T string);
    }
*/

    private final LayoutInflater mInflater;
//private Displayer<T> dist;
final private Function<T,String> dist;
//private Activity act;
    private List<T> ar;
    public RangeAdapter(List<T> ar,Context context, Function<T,String> dist) {
//	act=context;
        this.ar=ar;
       	mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    this.dist=dist;

    }
public void	setarray(List<T> arin)  {
	ar=arin;
	}
    @Override
    public View getDropDownView(int position, View convertView, ViewGroup parent) {
        if (convertView == null) {
            convertView = mInflater.inflate(android.R.layout.simple_spinner_dropdown_item, parent, false);
        }

	if(ar!=null) {
		T str=ar.get(position);
		((TextView)(convertView.findViewById(android.R.id.text1))).setText(dist.apply(str));
		}
	/*
	 Display display = act.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        int _width = size.x;
        convertView.setMaximumWidth(50);
	*/
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
    	if(ar==null)
		return 0;
	return ar.size();
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
           if (convertView == null) {
                convertView = mInflater.inflate(android.R.layout.simple_spinner_item, parent, false);
            }
	if(ar!=null) {
		  T str=ar.get(position);
		TextView thetext=convertView.findViewById(android.R.id.text1);
		thetext.setText(dist.apply(str));
	//        ((TextView)(convertView.findViewById(android.R.id.text1))).setText(str.toString());
		}


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
        return getCount()==0;
    }
}
