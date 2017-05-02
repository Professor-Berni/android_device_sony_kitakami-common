/*
 * Copyright (C) 2017 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.lineageos.sonyotgswitch;

import android.graphics.drawable.Icon;
import android.service.quicksettings.Tile;
import android.service.quicksettings.TileService;
import android.os.SystemProperties;
import android.widget.Toast;

import java.util.Timer;
import java.util.TimerTask;

import android.util.Log;

public class UsbOtgSwitch extends TileService {
    protected static final String OTG_SWITCH_ENABLED = "service.usb.otg.switch";
    protected static final String OTG_SWITCH_TIMEOUT = "service.usb.otg.switch.timeout";
    private int mTimeout = 30000; // Value in ms with a maximum of 60000ms. Default value in kernel = 30000ms
    private boolean mOtgDetection = false;
    private Timer mTimerTimeout = null;

    @Override
    public void onStartListening() {
        super.onStartListening();
        refresh();
    }

    @Override
    public void onClick() {
        super.onClick();
        SystemProperties.set(OTG_SWITCH_TIMEOUT, Integer.toString(mTimeout));
        SystemProperties.set(OTG_SWITCH_ENABLED, "check");
        mOtgDetection = true;
        Log.d("OtgSwitch", "Starting OTG detection for " + mTimeout + "ms");
        Toast.makeText(UsbOtgSwitch.this, getString(R.string.toast) + " " + mTimeout + " " + getString(R.string.units), Toast.LENGTH_LONG).show();
        refresh();
        if (mTimerTimeout != null) {
            mTimerTimeout.cancel();
        }
        mTimerTimeout = new Timer();
        mTimerTimeout.schedule(new TimerTask() {
            @Override
            public void run() {
                mOtgDetection = false;
                Log.d("OtgSwitch", "OTG detection stopped");
                refresh();
                mTimerTimeout.cancel();
                mTimerTimeout = null;
            }
        }, mTimeout, 1);
    }

    private void refresh() {
        if (mOtgDetection) {
            getQsTile().setIcon(Icon.createWithResource(this, R.drawable.ic_usb_otg_on));
            getQsTile().setState(Tile.STATE_ACTIVE);
        } else {
            getQsTile().setIcon(Icon.createWithResource(this, R.drawable.ic_usb_otg_off));
            getQsTile().setState(Tile.STATE_INACTIVE);
        }
        getQsTile().updateTile();
    }

}
