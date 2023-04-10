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
/*      Fri Jan 27 15:31:32 CET 2023                                                 */


package tk.glucodata.settings;

import android.app.Activity;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.recyclerview.widget.RecyclerView;
import tk.glucodata.MainActivity;
import tk.glucodata.Natives;

import static tk.glucodata.settings.Settings.float2string;

public class LabelListHolder extends RecyclerView.ViewHolder {

    public LabelListHolder(View view,LabelsClass set,View ok,Runnable onsave) {
        super(view);
       view.setOnClickListener(v -> {
		int pos=getAbsoluteAdapterPosition();

		set.mkchangelabel((MainActivity) v.getContext(),ok,onsave); //USE
		set.label.setText(set.labels.get(pos));
		if(set.garminwatch)
			set.labelprec.setText(float2string(Natives.getprec(pos)));
		set.labelweight.setText(float2string(Natives.getweight(pos)));
		   set.labelpos=pos;
		});

    }

}

