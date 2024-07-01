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


import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MyRenderer implements GLSurfaceView.Renderer {
       boolean started=false;
        int stepresult = 0;
        int badscan = 0;

        public void onDrawFrame(GL10 gl) {
            if (badscan!=0) {
			stepresult = Natives.badscan(badscan);
                badscan = 0;
            } else
                stepresult = Natives.step();

        }

public static int widthdiff=0;
        public void onSurfaceChanged(GL10 gl, int w, int h) {
//	eglSurfaceAttrib(eglGetCurrentDisplay(), eglGetCurrentSurface(EGL_DRAW), EGL_SWAP_BEHAVIOR , EGL_BUFFER_PRESERVED);
	if(Applic.Nativesloaded) {
           // Natives.resize(w, h-MainActivity.systembarBottom,Applic.initscreenwidth);
            Natives.resize(w, h,Applic.initscreenwidth);
	    widthdiff=w-Applic.initscreenwidth;
	    }
	   GlucoseCurve.setgeo(w,h);
        }

   public void onSurfaceCreated(GL10 gl, EGLConfig config) {
	if(Applic.Nativesloaded) {
		    Natives.initopengl(started);
		    started=true;
		    }
		    
        }
    }


