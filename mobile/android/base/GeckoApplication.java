/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.mozglue.GeckoLoader;
import org.mozilla.gecko.util.Clipboard;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Application;
import android.content.Context;
import android.content.res.Configuration;
import android.util.Log;

public class GeckoApplication extends Application {
    private static final String LOG_TAG = "GeckoApplication";

    private boolean mInBackground;
    private boolean mPausedGecko;

    private LightweightTheme mLightweightTheme;

    /**
     * We need to do locale work here, because we need to intercept
     * each hit to onConfigurationChanged.
     */
    @Override
    public void onConfigurationChanged(Configuration config) {
        Log.d(LOG_TAG, "onConfigurationChanged: " + config.locale +
                       ", background: " + mInBackground);

        // Do nothing if we're in the background. It'll simply cause a loop
        // (Bug 936756 Comment 11), and it's not necessary.
        if (mInBackground) {
            super.onConfigurationChanged(config);
            return;
        }

        // Otherwise, correct the locale. This catches some cases that GeckoApp
        // doesn't get a chance to.
        try {
            LocaleManager.correctLocale(getResources(), config);
        } catch (IllegalStateException ex) {
            // GeckoApp hasn't started, so we have no ContextGetter in LocaleManager.
            Log.w(LOG_TAG, "Couldn't correct locale.", ex);
        }

        super.onConfigurationChanged(config);
    }

    public void onActivityPause(GeckoActivityStatus activity) {
        mInBackground = true;

        if ((activity.isFinishing() == false) &&
            (activity.isGeckoActivityOpened() == false)) {
            // Notify Gecko that we are pausing; the cache service will be
            // shutdown, closing the disk cache cleanly. If the android
            // low memory killer subsequently kills us, the disk cache will
            // be left in a consistent state, avoiding costly cleanup and
            // re-creation. 
            GeckoAppShell.sendEventToGecko(GeckoEvent.createAppBackgroundingEvent());
            mPausedGecko = true;

            ThreadUtils.postToBackgroundThread(new Runnable() {
                @Override
                public void run() {
                    BrowserDB.expireHistory(getContentResolver(),
                                            BrowserContract.ExpirePriority.NORMAL);
                }
            });
        }
        GeckoConnectivityReceiver.getInstance().stop();
        GeckoNetworkManager.getInstance().stop();
    }

    public void onActivityResume(GeckoActivityStatus activity) {
        if (mPausedGecko) {
            GeckoAppShell.sendEventToGecko(GeckoEvent.createAppForegroundingEvent());
            mPausedGecko = false;
        }

        final Context applicationContext = getApplicationContext();
        GeckoBatteryManager.getInstance().start(applicationContext);
        GeckoConnectivityReceiver.getInstance().start(applicationContext);
        GeckoNetworkManager.getInstance().start(applicationContext);

        mInBackground = false;
    }

    @Override
    public void onCreate() {
        HardwareUtils.init(getApplicationContext());
        Clipboard.init(getApplicationContext());
        GeckoLoader.loadMozGlue();
        super.onCreate();
    }

    public boolean isApplicationInBackground() {
        return mInBackground;
    }

    public LightweightTheme getLightweightTheme() {
        return mLightweightTheme;
    }

    public void prepareLightweightTheme() {
        mLightweightTheme = new LightweightTheme(this);
    }
}
