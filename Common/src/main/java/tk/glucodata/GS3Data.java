package tk.glucodata;



import androidx.annotation.Keep;


@Keep
public final class GS3Data {
        public long result;
        public byte[] cmd;
        public String message;
        public long msecs;
        public GS3Data(long result, byte[] cmd,String message,long msecs) {
            this.result=result;
            this.cmd=cmd;
            this.message=message;
            this.msecs=msecs;
                }
        };

