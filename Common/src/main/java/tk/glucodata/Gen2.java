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
/*      Fri Jan 27 15:30:31 CET 2023                                                 */


package tk.glucodata;
import android.nfc.Tag;
import android.nfc.tech.NfcV;

import java.io.IOException;
import java.util.Arrays;

import tk.glucodata.AlgNfcV;

import static java.util.Arrays.copyOfRange;
import static tk.glucodata.AlgNfcV.goodnfc;
import static tk.glucodata.AlgNfcV.issuenfc;
import static tk.glucodata.Natives.V1;
import static tk.glucodata.Natives.V2;
import static tk.glucodata.Natives.USenabledStreaming;
import static tk.glucodata.Natives.abbottinit;

class Gen2 {
static final boolean newVersion=true;
static private final String LOG_ID="Gen2";
static      private byte[] lastv2(int i, int i2, int i3, byte[] bArr) {
        return V2(6520, i, new byte[]{(byte) i2, (byte) i3}, bArr);
    }
static    private byte[] readscanend(NfcV nfc,int start, int len,int extra) {
	Log.i(LOG_ID,"readscanend "+start+" "+len+" "+extra);
        int startinblocks = start / 8;
        int startblock = start % 8;
        int totlen = len + startblock;
        int blocks = (totlen + 7) / 8;
        int lenwholeblocks = blocks * 8;
        byte[] buf = new byte[lenwholeblocks];
        int iter = 0;
	int uitit=0;
        while (iter < blocks) {
            int curblock = startinblocks + iter;
            int min = Math.min(3, blocks - iter);
            byte[] received = issuenfc(nfc, new byte[]{2, -95, 7, 33, (byte) curblock, (byte) (min - 1)}) ;
            if (!goodnfc(received) || received.length < (((min+1) * 8) + 1)) {
                return null;
            }
	    int takelen= received.length - 9;
            System.arraycopy(received, 9, buf, uitit, takelen);
            uitit += takelen;
            iter+=min;
        }
        byte[] doorp2 = lastv2(extra, startinblocks, blocks, buf); //verschil
        return copyOfRange(doorp2, startblock, totlen);
    }





static  int v1check(int i, int i2, byte[] bArr) {
        return V1(29465, i, new byte[]{(byte) i2}, bArr);
    }

  static  private void FSLLFailure() {
    	Log.e(LOG_ID,"FSLLAuthFailure");
	}

//	byte[] uid = tag.getId();
static byte[] getuspatch(NfcV nfc, byte[] uid, int start, int len) {
 	if(!abbottinit()) {
		Log.e(LOG_ID,"abbottinit failed");
		return null;
		}
	final byte value=0x1f;
	byte[] outar=new byte[0x13];
	int res=scanner(nfc,uid, value, outar); 
	if(res<0)  {
		return null;
		}
	final byte[] transres=issuenfc(nfc,outar); //==static private byte[] nfccmd(final Tag tag, final byte[] cmd)
        if(!goodnfc(transres))  {
		errorP1(res);
		return null;
		}
	byte[] notfirst = copyOfRange(transres, 1, transres.length);

	int checked=v1check(res,0,notfirst);
	if(checked==-0xf) {
		Log.e(LOG_ID, "v1check(res,0,notfirst)==-0xf");
		FSLLFailure();
		if (res > 0) {
			errorP1(res);
		}

		return null;
		}
	else {
		if(checked!=-0x9) {
			if(checked>=0) {
				final byte[] nogeens=readscanend(nfc, start, len, res); 
			        if(res==0) {
					return nogeens;
					}
				errorP1(res);				
				return nogeens;
				}
			else {
			  Log.e(LOG_ID,"checked<0");
			    if(res<=0) {
			    	return null;
			    	}
			   errorP1(res);				
			   return null;
			    }
			}
		else {
		  Log.e(LOG_ID,"checked==-0x9");
     	  	    FSLLFailure(); 
		     if(res>0) {
		     	errorP1(res);				
		     	}
		      return null;

		    }
		}
	}	
static private final int scanner(NfcV nfc, byte[] uid, byte b, byte[] uitar) {
        try {
	 int numb=0;
	final byte always7=uid[6];
	if(newVersion) {
	    byte[] extra = issuenfc(nfc, new byte[]{2, -95, always7, 34});
	    if(!goodnfc(extra))  {
		Log.e(LOG_ID,"new byte[]{2, -95, 7, 34}) failed");
		return -1;
		}
	    numb = (extra[3] | (extra[4] << 8)) & 0xFFFF;
	    }
         byte[] a = issuenfc(nfc, new byte[]{2, -95, always7, 32});
        if(goodnfc(a)) {
	        Log.d(LOG_ID,"scanner goodnfc");
        	byte[] result = copyOfRange(a, 1, a.length);
        	return v1v2addcommand(b, numb,uid, result, uitar);
	        }
	    else {
			if(a==null)
				Log.d(LOG_ID,"issuenfc returned null");
			else
				Log.d(LOG_ID,"issuenfc returned "+a.toString());
			return -1;
		}

        } catch (Throwable error) {
		String mess=error!=null?error.getMessage():null;
		if(mess==null) {
			mess="error";
			}
	       Log.stack(LOG_ID ,mess,error);
        }
	return -1;
    }

  static   int errorP1(int i) {
        return V1(37400, i, null, null);
    }
static    private int v1v2addcommand(int i, int numb,byte[] bArr, byte[] bArr2, byte[] bArrout) {
	Log.d(LOG_ID, "v1v2addcommand("+i+" ...)");
        byte[] bArr4 = new byte[2];
        int resP1 = V1(28960, numb, bArr, null);
        if (resP1 < 0) {
            return resP1;
        }
        bArr4[0] = 0;
        byte b = (byte) i;
        bArr4[1] = b;
        byte[] P2res = V2(6440, resP1, bArr4, bArr2);
        if (P2res == null) {
            errorP1(resP1);
            return -99;
        }
        System.arraycopy(P2res, 0, bArrout, 0, P2res.length);
        bArrout[0] = 2;
        bArrout[1] = -95;
        bArrout[2] = 7;
        bArrout[3] = b;
        return resP1;
    }
//non us  info  [-99, 8, 48, 1, 113, 54]
//9D 08 30 01 71 36 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 -099 0008 0048 0001 0113 0054
//30
//usinfo [118, 9, 57, 2, 51, 18]
//76 09 39 02 33 12 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 0118 0009 0057 0002 0051 0018
//39
// was public final int m8472h(byte[] bArr) 



/*
static private final int getversionjava(byte[] info) {
        if (info != null && info.length >= 6) {
            int i = info[2] >> 4;
            int i2 = info[2] & 0xF;
            if (i == 3) {
                if (i2 < 9) {
                    return 1;
                }
                return 2;
            } else if (i == 7) {
                if (i2 < 4) {
                    return 1;
                }
                return 2;
            }
        }
        return 0;
    } */

static final int getversion(byte[] info) {
 	return Natives.getinfogen(info);
	}
/*
static final void testgen() {
	byte[] info={0x0,0x0,0x0,0x0,0x0,0x0};
	Log.i(LOG_ID,"Test generations");
	for(int i=0;i<=0xFF;i++) {
		info[2]=(byte)i;
		getversion(info);
		}
	}
*/
private static int v2v1streaming(int i, int i2, byte[] bArr, byte[] bArr2) {
        byte[] P2res = Natives.V2(22321, i, new byte[]{(byte) i2, (byte) bArr2.length}, bArr);
        if (P2res == null) {
            Natives.V1(37400, i, null, null);
            return -99;
        }
        System.arraycopy(P2res, 0, bArr2, 0, P2res.length);
        return bArr2.length;
    }

static private boolean lastenaable(int i, byte[] bArr, byte[] bArr2, byte[] bArr3) {
        byte[] bArr4 = new byte[9];
        byte[] bArr5 = new byte[1];
        if (v2v1streaming(i, 0, bArr, bArr4) < 0) {
            return false;
        }
	if(newVersion) {
		bArr5[0] = 9;
		byte[] P2res = Natives.V2(6145, i, bArr5, null);
		if (P2res == null) {
		    return false;
		     }
		Log.i(LOG_ID,"P2res.length="+P2res.length);
		System.arraycopy(bArr4, 0, bArr3, 0, 6);

		var begin=P2res.length-2;
		System.arraycopy(P2res, 0, bArr2, 0, begin);
		System.arraycopy(bArr4, 6, bArr2, begin, 3);
		System.arraycopy(P2res, begin, bArr2, begin+3, 2);
		return true;
		}
	else {
		bArr5[0] = 7;
		byte[] P2res = Natives.V2(6145, i, bArr5, null);
		if (P2res == null) {
		    return false;
			}
		System.arraycopy(bArr4, 0, bArr3, 0, 6);


		System.arraycopy(P2res, 0, bArr2, 0, P2res.length);
		System.arraycopy(bArr4, 6, bArr2, P2res.length, 3);
		return true;
		}
    }
private  static boolean inactivate(NfcV nfc,byte[] uid) {
        byte[] outarv2=new  byte[0x13];
        byte value=27;
        Log.d(LOG_ID,"activate");
        int resv1=scanner(nfc,uid, value, outarv2);
      if(resv1<0)	return false;
      final byte[] resp1=issuenfc(nfc,outarv2);      
      final boolean res=resp1!=null &&(v2v1streaming(resv1,0, Arrays.copyOfRange(resp1, 1, resp1.length), new byte[5]) > 0);
       errorP1(resv1);
       return res;
	}

static boolean activate(final Tag tag) {
	try {
		byte[] uid=tag.getId();
		final NfcV nfc = NfcV.get(tag);
       nfc.connect();
		boolean res=inactivate(nfc, uid);
		nfc.close();
		return res;
		}
  catch (Throwable error) {
		String mess=error!=null?error.getMessage():null;
		if(mess==null) {
			mess="error";
			}
	       Log.stack(LOG_ID ,mess,error);
		}
	return false;
	}
private static boolean gen2enablestreaming(NfcV nfc,byte[] uid) {
	byte[] outarv2=new  byte[0x13];
	byte value=0x1e;
	Log.d(LOG_ID,"gen2enablestreaming");
	int resv1=scanner(nfc,uid, value, outarv2); 
	Log.d(LOG_ID,"scanner="+resv1);
	if(resv1>=0) {
		    byte[] resp1=issuenfc(nfc,outarv2);      
 
		    if(!goodnfc(resp1)) {
  			FSLLFailure();
			if(resv1>0) {
  				errorP1(resv1);
				}
			return false;
		    	}
		    else {
		       byte[] tail=copyOfRange(resp1,1,resp1.length);

			 byte[] streamingAuthenticationData=new byte[(newVersion?12:10)];  
	
			byte[] bleAddress =new  byte[6]; 
			if(lastenaable(resv1,tail,streamingAuthenticationData,bleAddress)) {
				if(resv1>0) { 
  					errorP1(resv1);
					}
				USenabledStreaming(uid,streamingAuthenticationData,bleAddress);
				return true;
				}
			else {
  				FSLLFailure();
				return false;
				}
			}
		    }
	else
		return false;
	}

static boolean gen2enablestreaming(final Tag tag) {
	try {
		byte[] uid=tag.getId();
		final NfcV nfc = NfcV.get(tag);
       nfc.connect();
		boolean res=gen2enablestreaming(nfc, uid);
		nfc.close();
		return res;
		}
  catch (Throwable error) {
		String mess=error!=null?error.getMessage():null;
		if(mess==null) {
			mess="error";
			}
	       Log.stack(LOG_ID ,mess,error);
		}
	return false;
	}
}
