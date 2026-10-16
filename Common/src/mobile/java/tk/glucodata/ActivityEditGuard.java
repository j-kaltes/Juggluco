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

/** MainActivity removes a back callback before running it. Restore it while asking. */
final class ActivityEditGuard implements Runnable {
    interface BackStack {void push(Runnable callback);void pop();}
    interface Prompt {void show(Runnable discard,Runnable dismissed);}
    private final BooleanSupplier changed;
    private final BackStack back;
    private final Prompt prompt;
    private final Runnable close;
    private boolean asking;
    ActivityEditGuard(BooleanSupplier changed,BackStack back,Prompt prompt,Runnable close) {
        this.changed=changed;this.back=back;this.prompt=prompt;this.close=close;
    }
    @Override public void run() {
        if(!changed.getAsBoolean()){close.run();return;}
        back.push(this);
        if(asking)return;
        asking=true;
        prompt.show(()->{back.pop();close.run();},()->asking=false);
    }
}
