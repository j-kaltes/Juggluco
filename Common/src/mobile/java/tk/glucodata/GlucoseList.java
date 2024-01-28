package tk.glucodata;

import static android.health.connect.datatypes.Metadata.RECORDING_METHOD_UNKNOWN;

import android.os.Build;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.health.connect.client.records.BloodGlucoseRecord;
import androidx.health.connect.client.records.metadata.Device;
import androidx.health.connect.client.records.metadata.Metadata;

import java.time.Instant;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;

public class GlucoseList implements List<BloodGlucoseRecord>   {
    Metadata metadata;


    private static final String LOG_ID="GlucoseList";
   long sensorptr;
   int start;
    int len;
public    GlucoseList(Metadata meta,long sensorptr,int start,int len) {
        this.metadata=meta;
    this.sensorptr=sensorptr;
        this.start=start;
        this.len=len;

    }
  public void setsizes(int start,int len) {
  	this.start=start;
	this.len=len;
  	}

    @Override
    public int size() { 
    	Log.i(LOG_ID,"size()="+len);
    	return len; 
    	}

    @Override
    public boolean isEmpty() {
        return len!=0;
    }

    @Override
    public boolean contains(@Nullable Object o) {
        return false;
    }

    @NonNull
    @Override
    public Iterator<BloodGlucoseRecord> iterator() {
        return new GlucoseIterator(this,0);
    }

    @NonNull
    @Override
    public Object[] toArray() {
        return new Object[0];
    }

    @NonNull
    @Override
    public <T> T[] toArray(@NonNull T[] a) {
        return null;
    }

    @Override
    public boolean add(BloodGlucoseRecord bloodGlucoseRecord) {
        return false;
    }

    @Override
    public boolean remove(@Nullable Object o) {
        return false;
    }

    @Override
    public boolean containsAll(@NonNull Collection<?> c) {
        return false;
    }

    @Override
    public boolean addAll(@NonNull Collection<? extends BloodGlucoseRecord> c) {
        return false;
    }

    @Override
    public boolean addAll(int index, @NonNull Collection<? extends BloodGlucoseRecord> c) {
        return false;
    }

    @Override
    public boolean removeAll(@NonNull Collection<?> c) {
        return false;
    }

    @Override
    public boolean retainAll(@NonNull Collection<?> c) {
        return false;
    }

    @Override
    public void clear() {

    }

    @Override
    public BloodGlucoseRecord get(int index) {
        return null;
    }

    @Override
    public BloodGlucoseRecord set(int index, BloodGlucoseRecord element) {
        return null;
    }

    @Override
    public void add(int index, BloodGlucoseRecord element) {

    }

    @Override
    public BloodGlucoseRecord remove(int index) {
        return null;
    }

    @Override
    public int indexOf(@Nullable Object o) {
        return 0;
    }

    @Override
    public int lastIndexOf(@Nullable Object o) {
        return 0;
    }

    @NonNull
    @Override
    public ListIterator<BloodGlucoseRecord> listIterator() {
        return new GlucoseIterator(this,0);
    }

    @NonNull
    @Override
    public ListIterator<BloodGlucoseRecord> listIterator(int index) {
        return new GlucoseIterator(this,index);
    }

    @NonNull
    @Override
    public List<BloodGlucoseRecord> subList(int fromIndex, int toIndex) {
        return null;
    }

}
