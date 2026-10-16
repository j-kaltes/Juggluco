/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
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
/*      Thu Oct 15 12:44:39 CEST 2026                                                */

// SPDX-License-Identifier: GPL-3.0-or-later
package tk.glucodata;

import android.graphics.Bitmap;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.webkit.SslErrorHandler;
import android.webkit.WebResourceRequest;
import android.webkit.WebResourceError;
import android.webkit.WebResourceResponse;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.EditText;
import java.io.ByteArrayInputStream;
import java.util.Collections;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;
import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static tk.glucodata.settings.Settings.removeContentView;
import static tk.glucodata.util.getbutton;
import static tk.glucodata.util.getlabel;

/** Dexcom owns the credential form. Tokens and all API HTTP remain in C++. */
final class ClarityLogin {
    private static final String LOG_ID="ClarityLogin";
    // A closed view can still have a native token exchange in progress.
    private static final AtomicBoolean finishing=new AtomicBoolean();
    private final MainActivity act;
    private final View parent;
    private final boolean secureBefore;
    private final int softInputBefore;
    private final Runnable refreshed;
    private WebView web;
    private String country;
    private boolean busy, callbackSeen, completionAttempted, closed;
    private int backDepth;

    private ClarityLogin(MainActivity act,View parent,Runnable refreshed) {
        this.act=act;
        this.parent=parent;
        this.refreshed=refreshed;

        var attributes=act.getWindow().getAttributes();
        secureBefore=(attributes.flags & WindowManager.LayoutParams.FLAG_SECURE)!=0;
        softInputBefore=attributes.softInputMode;

    }
private void showlogin(String country,boolean finishPending) {
        this.country=country;
        act.getWindow().addFlags(WindowManager.LayoutParams.FLAG_SECURE);
        act.getWindow().setSoftInputMode((softInputBefore & ~WindowManager.LayoutParams.SOFT_INPUT_MASK_ADJUST) | WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        web=new WebView(act);
        web.setLayoutParams(new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
        var settings=web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(false);
        settings.setAllowContentAccess(false);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        settings.setCacheMode(WebSettings.LOAD_NO_CACHE);
        settings.setSaveFormData(false);

        web.setWebViewClient(new WebViewClient() {
            @Override public boolean shouldOverrideUrlLoading(WebView view,WebResourceRequest request) {
                return request.isForMainFrame() && navigate(request.getUrl());
            }
            @Override public boolean shouldOverrideUrlLoading(WebView view,String url) {
                return navigate(Uri.parse(url));
            }
            @Override public WebResourceResponse shouldInterceptRequest(WebView view,WebResourceRequest request) {
                if("https".equals(request.getUrl().getScheme()))return null;
                Log.e(LOG_ID,"Blocked non-HTTPS sign-in resource");
                return new WebResourceResponse("text/plain","UTF-8",403,"Forbidden",
                    Collections.emptyMap(),new ByteArrayInputStream(new byte[0]));
            }
            @Override public void onReceivedSslError(WebView view,SslErrorHandler handler,SslError error) {
                handler.cancel();
                Log.e(LOG_ID,"Sign-in TLS certificate error code="+error.getPrimaryError());
                showError(act.getString(R.string.clarity_login_tls_error));
            }
            @Override public void onReceivedError(WebView view,WebResourceRequest request,WebResourceError error) {
                Log.e(LOG_ID,"Sign-in resource error code="+error.getErrorCode()+" mainFrame="+request.isForMainFrame());
                if(request.isForMainFrame())showError(act.getString(R.string.clarity_login_load_error));
            }
            @Override public void onReceivedError(WebView view,int code,String description,String failingUrl) {
                Log.e(LOG_ID,"Sign-in page error code="+code);
                showError(act.getString(R.string.clarity_login_load_error));
            }
            @Override public void onReceivedHttpError(WebView view,WebResourceRequest request,WebResourceResponse response) {
                Log.e(LOG_ID,"Sign-in HTTP error="+response.getStatusCode()+" mainFrame="+request.isForMainFrame());
                if(request.isForMainFrame())showError(act.getString(R.string.clarity_login_http_error,response.getStatusCode()));
            }
            @Override public void onPageStarted(WebView view,String url,Bitmap favicon) {
                if(url.startsWith("dexcomg7:")){view.stopLoading();navigate(Uri.parse(url));}
            }
        });
        web.addOnAttachStateChangeListener(new View.OnAttachStateChangeListener() {
            @Override public void onViewAttachedToWindow(View view) {}
            @Override public void onViewDetachedFromWindow(View view) { release(); }
        });
        act.addMyContentView(web,new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
        MainActivity.setonback(()->{
            release();
            if(!act.isFinishing() && !act.isDestroyed())refreshed.run();
            });
        backDepth=MainActivity.onbacknr();
        if(closed || busy)return;
        if(finishing.get()){
            showError(act.getString(R.string.clarity_login_in_progress));
            return;
            }
        if(finishPending) {
            complete("");
            return;
            }
        try {
            String[] result=Natives.clarityLoginBegin(country,Locale.getDefault().toLanguageTag(),Build.MANUFACTURER,Build.MODEL,Build.VERSION.RELEASE);
            if(result==null || result.length!=2)throw new IllegalStateException();
            if(!result[1].isEmpty()){showError(result[1]);return;}
            String url=result[0];
            if(!dexcomPage(Uri.parse(url)))throw new IllegalArgumentException();
            callbackSeen=false;
            web.loadUrl(url);
        } catch(Exception ex) {
            Clarity.logFailure(LOG_ID,"Begin sign-in",ex);
            showError(act.getString(R.string.clarity_login_start_error));
        }
    }


static private void doLogin(MainActivity act,View parent,Runnable refreshed,String country,boolean finishPending) {
        ClarityLogin login=null;
        try {
            login=new ClarityLogin(act,parent,refreshed);
            login.showlogin(country,finishPending);
            }
        catch(Exception ex) {
            Clarity.logFailure(LOG_ID,"Open sign-in",ex);
            if(login!=null) {
                if(login.backDepth>0 && MainActivity.onbacknr()==login.backDepth)
                    MainActivity.doonback();
                else
                    login.release();
                }
            if(!act.isFinishing() && !act.isDestroyed())
                show(act,parent,refreshed,country,act.getString(R.string.clarity_login_start_error),finishPending);
            }
        }
static void show(MainActivity act,View parent,Runnable refreshed) {
        show(act,parent,refreshed,tk.glucodata.util.getCountry(),act.getString(R.string.clarity_login_instructions),false);
        }
static private void show(MainActivity act,View parent,Runnable refreshed,String savedCountry,String text,boolean finishPending) {
        var message=getlabel(act,text);
        var label=getlabel(act,act.getString(R.string.clarity_login_country));
        var country=new EditText(act);
        country.setSingleLine(true);
        country.setText(savedCountry);
        var begin=getbutton(act,R.string.clarity_sign_in);
        var cancel=getbutton(act,R.string.cancel);
        // Retry belongs to the setup/error screen, never above the web page.
        var retry=finishPending?getbutton(act,R.string.clarity_login_finish_pending):null;
        final Layout root;
        message.setLayoutParams(new ViewGroup.LayoutParams(  MATCH_PARENT, WRAP_CONTENT));
        if(retry==null)
            root=new Layout(act,new View[]{message},new View[]{label,country},new View[]{begin,cancel})
                .portraitLayout(new View[]{message},new View[]{label},new View[]{country},new View[]{begin}, new View[]{cancel});
        else
            root=new Layout(act,new View[]{message},new View[]{label,country},new View[]{begin,cancel},new View[]{retry})
                .portraitLayout(new View[]{message},new View[]{label},new View[]{country},new View[]{begin,cancel},new View[]{retry});

        begin.setOnClickListener(v->startFromSetup(act,root,parent,refreshed,country.getText().toString(),false));
        if(retry!=null)
            retry.setOnClickListener(v->startFromSetup(act,root,parent,refreshed,country.getText().toString(),true));

        root.setBackgroundColor(Applic.backgroundcolor);

        final int sidepad=(int)(GlucoseCurve.metrics.density*30);
        root.systembarPadding((left,top,right,bottom)-> {
              return   new int[]{left+sidepad,top+sidepad,sidepad+right,bottom+sidepad};
              });
        parent.setVisibility(View.GONE);
        act.addMyContentView(root,new ViewGroup.LayoutParams(MATCH_PARENT,MATCH_PARENT));
        MainActivity.setonback(()->{
            removeContentView(root);
            parent.setVisibility(View.VISIBLE);
            if(!act.isFinishing() && !act.isDestroyed())refreshed.run();
           });
        cancel.setOnClickListener(v->MainActivity.doonback());
    }
static private void startFromSetup(MainActivity act,Layout root,View parent,Runnable refreshed,String country,boolean finishPending) {
        if(root.getParent()==null || act.isFinishing() || act.isDestroyed())return;
        if(finishing.get()) {
            Applic.Toaster(act.getString(R.string.clarity_login_in_progress));
            return;
            }
        removeContentView(root);
        MainActivity.poponback();
        doLogin(act,parent,refreshed,country.trim().toUpperCase(Locale.ROOT),finishPending);
    }
    private static boolean dexcomPage(Uri uri) {
        String host=uri.getHost();
        if(host==null)return false;
        host=host.toLowerCase(Locale.ROOT);
        return "https".equals(uri.getScheme()) && uri.getUserInfo()==null &&
            (uri.getPort()==-1 || uri.getPort()==443) &&
            (host.equals("dexcom.com") || host.endsWith(".dexcom.com") ||
             host.equals("dexcom.eu") || host.endsWith(".dexcom.eu"));
    }
    private boolean navigate(Uri uri) {
        if(closed || busy)return true;
        if("dexcomg7".equals(uri.getScheme())) {
            if(!callbackSeen){callbackSeen=true;complete(uri.toString());}
            return true;
        }
        if(dexcomPage(uri))return false;
        showError(act.getString(R.string.clarity_login_destination_error));
        return true;
    }
    private void complete(String callback) {
        if(closed || busy)return;
        if(!finishing.compareAndSet(false,true)) {
            showError(act.getString(R.string.clarity_login_in_progress));
            return;
            }
        busy=true;
        completionAttempted=true;
        web.stopLoading();
        web.setEnabled(false);
        Applic.Toaster(act.getString(R.string.clarity_login_completing));
        var task=new Thread(()->{
            String error;
            try {
                error=Natives.clarityLoginFinish(callback);
                if(error==null)
                        throw new IllegalStateException();
            } catch(Exception ex) {
                Clarity.logFailure(LOG_ID,"Complete sign-in",ex);
                error=act.getString(R.string.clarity_login_complete_error);
                }    
            finally { 
                finishing.set(false); 
                }
            final String result=error;
            act.runOnUiThread(()->{
                busy=false;
                if(closed || act.isFinishing() || act.isDestroyed())return;
                web.setEnabled(true);
                if(result.isEmpty()) {
                    if(MainActivity.onbacknr()==backDepth)MainActivity.doonback();
                    else Applic.Toaster(act.getString(R.string.clarity_login_success));
                }
                else showError(act.getString(R.string.clarity_login_retry_error,result));
            });
        },"Clarity sign-in");
        try { task.start(); }
        catch(Exception ex) {
            finishing.set(false);
            busy=false;
            web.setEnabled(true);
            Clarity.logFailure(LOG_ID,"Start sign-in task",ex);
            showError(act.getString(R.string.clarity_login_complete_error));
           }
       }
    private void showError(String text) {
        Log.e(LOG_ID,text); // Fixed UI/native Error diagnostics only, never redirect URLs.
        if(closed || busy || act.isFinishing() || act.isDestroyed())return;
        if(MainActivity.onbacknr()!=backDepth) {
            // Do not pop a different screen that was opened above login.
            Applic.Toaster(text);
            return;
            }
        MainActivity.doonback();
        show(act,parent,refreshed,country,text,completionAttempted);
        }
    private void release() {
        if(closed)
            return;
        closed=true;
        if(web!=null) {
            try {
                web.stopLoading();
                removeContentView(web);
                web.destroy();
            } catch(Exception ex) { Clarity.logFailure(LOG_ID,"Release sign-in WebView",ex); }
            finally { web=null; }
        }
        if(!secureBefore)act.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_SECURE);
        act.getWindow().setSoftInputMode(softInputBefore);
        parent.setVisibility(View.VISIBLE);
    }
}
