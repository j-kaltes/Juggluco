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
/*      Thu Mar 28 20:14:36 CET 2024                                                 */


package tk.glucodata;

import java.util.UUID;

class AidexXGattCallback extends SuperGattCallback {

    static final UUID ScanServiceUUID = UUID.fromString("00000000-0000-0000-0000-000000000000");

    AidexXGattCallback(String SerialNumber, long dataptr) {
        super(SerialNumber, dataptr, 0x50);
    }

    public void startUnpair(UnpairOverlayHost host, java.util.function.Predicate<Boolean> ended) {
    }

    public void startClear(UnpairOverlayHost host, java.util.function.Predicate<Boolean> ended) {
    }

    public static String deviceName2name(String deviceName) {
        return "";
    }

    static boolean addbyDeviceName(MainActivity act, String deviceName) {
        return false;
    }
}



