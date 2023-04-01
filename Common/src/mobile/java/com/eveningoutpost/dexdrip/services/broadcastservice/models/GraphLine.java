package com.eveningoutpost.dexdrip.services.broadcastservice.models;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.Keep;

import java.util.ArrayList;
import java.util.List;

import lecho.lib.hellocharts.model.Line;
import lecho.lib.hellocharts.model.PointValue;
import lombok.Getter;
import lombok.Setter;
@Keep
public class GraphLine implements Parcelable {
    public static final Creator<GraphLine> CREATOR = new Creator<GraphLine>() {

        @Override
        public GraphLine createFromParcel(Parcel source) {
            return new GraphLine(source);
        }

        @Override
        public GraphLine[] newArray(int size) {
            return new GraphLine[size];
        }
    };
    @Getter
    @Setter
    public List<GraphPoint> values;
    @Getter
    @Setter
    private int color;

    public GraphLine(int col) {
        values = new ArrayList<>();
        color = col;
    }
    public GraphLine() {
    	this(0);
    }
@Keep
 public void add(float x,float y) {
          values.add(new GraphPoint(x,y));
 	}
	/*
    public GraphLine(Line line) {
        values = new ArrayList<>();
        line.update(0);
        for (PointValue pointValue : line.getValues()) {
            values.add(new GraphPoint(pointValue.getX(), pointValue.getY()));
        }
        color = line.getColor();
    }     public GraphLine(Line line) {
        values = new ArrayList<>();
        line.update(0);
        for (PointValue pointValue : line.getValues()) {
            double real_timestamp = pointValue.getX();
            values.add(new GraphPoint((float)real_timestamp, (float)pointValue.getY()));
        }
        color = line.getColor();
    }
*/

    public GraphLine(Parcel parcel) {
        values = parcel.readArrayList(GraphPoint.class.getClassLoader());
        color = parcel.readInt();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeList(values);
        parcel.writeInt(color);
    }
}
