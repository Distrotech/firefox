/* -*- Mode: Java; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.util.Log;

import java.util.Locale;

/**
 * This class manages persistence, application, and otherwise handling of
 * user-specified locales.
 *
 * Of note:
 * 
 * * It's a singleton, because its scope extends to that of the application,
 *   and definitionally all changes to the locale of the app must go through
 *   this.
 * * It's lazy.
 * * It has ties into the Gecko event system, because it has to tell Gecko when
 *   to switch locale.
 * * It relies on using the SharedPreferences file owned by the browser (in
 *   Fennec's case, "GeckoApp") for performance.
 */
public class LocaleManager {
    private static final String LOG_TAG = "GeckoLocales";

    // These are both volatile because we don't impose restrictions
    // over which thread calls our methods.
    private static volatile ContextGetter getter = null;
    private static volatile Locale currentLocale = null;

    private static volatile boolean inited = false;
    private static boolean systemLocaleDidChange = false;
    private static BroadcastReceiver receiver;

    public static void setContextGetter(ContextGetter getter) {
        Log.d(LOG_TAG, "Calling setContextGetter: " + getter);
        LocaleManager.getter = getter;
    }

    public static void initialize() {
        if (inited) {
            return;
        }

        receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                systemLocaleDidChange = true;
            }
        };
        getContext().registerReceiver(receiver, new IntentFilter(Intent.ACTION_LOCALE_CHANGED));
        inited = true;
    }

    public static boolean systemLocaleDidChange() {
        return systemLocaleDidChange;
    }

    private static Context getContext() {
        if (getter == null) {
            throw new IllegalStateException("No ContextGetter; cannot fetch context.");
        }
        return getter.getContext();
    }

    private static SharedPreferences getSharedPreferences() {
        if (getter == null) {
            throw new IllegalStateException("No ContextGetter; cannot fetch prefs.", new RuntimeException("No prefs."));
        }
        return getter.getSharedPreferences();
    }

    /**
     * Every time the system gives us a new configuration, it
     * carries the external locale. Fix it.
     */
    public static void correctLocale(Resources res, Configuration config) {
        Locale current = getCurrentLocale();
        if (current == null) {
            return;
        }

        // I know it's tempting to short-circuit here if the config seems to be
        // up-to-date, but the rest is necessary.

        config.locale = current;

        // The following two lines are heavily commented in case someone
        // decides to chase down performance improvements and decides to
        // question what's going on here.
        // Both lines should be cheap, *but*...

        // This is unnecessary for basic string choice, but it almost
        // certainly comes into play when rendering numbers, deciding on RTL,
        // etc. Take it out if you can prove that's not the case.
        Locale.setDefault(current);

        // This seems to be a no-op, but every piece of documentation under the
        // sun suggests that it's necessary, and it certainly makes sense.
        res.updateConfiguration(config, res.getDisplayMetrics());
    }

    private static Locale parseLocaleCode(final String localeCode) {
        int index;
        if ((index = localeCode.indexOf('-')) != -1 ||
            (index = localeCode.indexOf('_')) != -1) {
            final String langCode = localeCode.substring(0, index);
            final String countryCode = localeCode.substring(index + 1);
            return new Locale(langCode, countryCode);
        } else {
            return new Locale(localeCode);
        }
    }

    /**
     * Gecko uses locale codes like "es-ES", whereas a Java {@link Locale}
     * stringifies as "es_ES".
     *
     * This method approximates the Java 7 method <code>Locale#toLanguageTag()</code>.
     *
     * @return a locale string suitable for passing to Gecko.
     */
    public static String getLanguageTag(final Locale locale) {
        // If this were Java 7:
        // return locale.toLanguageTag();

        String language = locale.getLanguage();  // Can, but should never be, an empty string.
        // Modernize certain language codes.
        if (language.equals("iw")) {
            language = "he";
        } else if (language.equals("in")) {
            language = "id";
        } else if (language.equals("ji")) {
            language = "yi";
        }

        String country = locale.getCountry();    // Can be an empty string.
        if (country.equals("")) {
            return language;
        }
        return language + "-" + country;
    }

    public static Locale getCurrentLocale() {
        if (currentLocale != null) {
            return currentLocale;
        }

        final String current = getPersistedLocale();
        if (current == null) {
            return null;
        }
        return currentLocale = parseLocaleCode(current);
    }

    /**
     * Updates the Java locale and the Android configuration.
     *
     * Returns the persisted locale if it differed.
     *
     * Does not notify Gecko.
     */
    private static String updateLocale(String localeCode) {
        // Fast path.
        final Locale defaultLocale = Locale.getDefault();
        if (defaultLocale.toString().equals(localeCode)) {
            return null;
        }

        final Locale locale = parseLocaleCode(localeCode);

        // Fast path.
        if (defaultLocale.equals(locale)) {
            return null;
        }

        Locale.setDefault(locale);
        currentLocale = locale;

        // Update resources.
        Resources res = getContext().getResources();
        Configuration config = res.getConfiguration();
        config.locale = locale;
        res.updateConfiguration(config, res.getDisplayMetrics());

        return locale.toString();
    }

    public static void notifyGeckoOfLocaleChange(Locale locale) {
        // Tell Gecko.
        GeckoEvent ev = GeckoEvent.createBroadcastEvent("Locale:Changed", getLanguageTag(locale));
        GeckoAppShell.sendEventToGecko(ev);
    }

    private static String getPrefName() {
        return getContext().getPackageName() + ".locale";
    }

    public static String getPersistedLocale() {
        final SharedPreferences settings = getSharedPreferences();

        // N.B., it is expected that any per-profile settings will be
        // implemented via SharedPreferences multiplexing in ContextGetter, not
        // via profile-annotated preference names.
        final String locale = settings.getString(getPrefName(), "");

        if ("".equals(locale)) {
            return null;
        }
        return locale;
    }

    private static void persistLocale(String localeCode) {
        final SharedPreferences settings = getSharedPreferences();
        settings.edit().putString(getPrefName(), localeCode).commit();
    }

    public static String getAndApplyPersistedLocale() {
        final long t1 = android.os.SystemClock.uptimeMillis();
        final String localeCode = getPersistedLocale();
        if (localeCode == null) {
            return null;
        }

        // Note that we don't tell Gecko about this. We notify Gecko when the
        // locale is set, not when we update Java.
        updateLocale(localeCode);

        final long t2 = android.os.SystemClock.uptimeMillis();
        Log.i(LOG_TAG, "Locale read and update took: " + (t2 - t1) + "ms.");
        return localeCode;
    }

    /**
     * Returns the set locale if it changed.
     *
     * Always persists and notifies Gecko.
     */
    public static String setSelectedLocale(String localeCode) {
        final String resultant = updateLocale(localeCode);

        // We always persist and notify Gecko, even if nothing seemed to
        // change. This might happen if you're picking a locale that's the same
        // as the current OS locale. The OS locale might change next time we
        // launch, and we need the Gecko pref and persisted locale to have been
        // set by the time that happens.
        persistLocale(localeCode);
        notifyGeckoOfLocaleChange(getCurrentLocale());
        return resultant;
    }
}

