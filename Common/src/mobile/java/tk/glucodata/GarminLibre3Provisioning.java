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

/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */

package tk.glucodata;

import java.util.ArrayList;
import java.util.List;

/** Builds the one-time provisioning record for the direct-BLE Libre 3 Garmin app. */
public final class GarminLibre3Provisioning {
    private static final int SECRET_SIZE = 180; // PIN[4] + challenge context[176]
    private static final int RECORD_SIZE = 191;

    private GarminLibre3Provisioning() {}

    /**
     * Connect IQ 3.1 supports List/Number directly.  Keep the provisioning data
     * binary in Juggluco and expose each unsigned byte as one Integer; no Base64.
     */
    public static List<Integer> makeMessage(String bluetoothAddress, byte[] secret180) {
        if (bluetoothAddress == null)
            throw new IllegalArgumentException("Bluetooth address is null");
        if (secret180 == null || secret180.length != SECRET_SIZE)
            throw new IllegalArgumentException("Libre 3 Garmin secret must be 180 bytes");

        final String[] fields = bluetoothAddress.split(":");
        if (fields.length != 6)
            throw new IllegalArgumentException("Invalid Bluetooth address");

        final byte[] record = new byte[RECORD_SIZE];
        record[0] = 'L';
        record[1] = '3';
        record[2] = 'G';
        record[3] = '2';
        record[4] = 1;

        for (int i = 0; i < 6; ++i) {
            if (fields[i].length() != 2)
                throw new IllegalArgumentException("Invalid Bluetooth address");
            final int value;
            try {
                value = Integer.parseInt(fields[i], 16);
            } catch (NumberFormatException ex) {
                throw new IllegalArgumentException("Invalid Bluetooth address", ex);
            }
            record[5 + i] = (byte)value;
        }

        System.arraycopy(secret180, 0, record, 11, SECRET_SIZE);

        final ArrayList<Integer> message = new ArrayList<>(RECORD_SIZE);
        for (byte value : record)
            message.add(value & 0xff);
        return message;
    }
}
