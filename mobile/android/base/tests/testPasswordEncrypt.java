package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import org.mozilla.gecko.db.BrowserContract;

import android.content.ContentValues;
import android.content.ContentResolver;
import android.database.Cursor;
import android.content.Context;
import android.net.Uri;
import java.io.File;

import org.json.JSONArray;
import org.json.JSONObject;

public class testPasswordEncrypt extends BaseTest {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testPasswordEncrypt() {
        Context context = (Context)getActivity();
        ContentResolver cr = context.getContentResolver();
        mAsserter.isnot(cr, null, "Found a content resolver");
        ContentValues cvs = new ContentValues();

        blockForGeckoReady();

        File db = new File(mProfile, "signons.sqlite");
        String dbPath = db.getPath();

        Uri passwordUri;
        cvs.put("hostname", "http://www.example.com");
        cvs.put("encryptedUsername", "username");
        cvs.put("encryptedPassword", "password");

        // Attempt to insert into the db
        passwordUri = BrowserContract.Passwords.CONTENT_URI;
        Uri.Builder builder = passwordUri.buildUpon();
        passwordUri = builder.appendQueryParameter("profilePath", mProfile).build();

        Uri uri = cr.insert(passwordUri, cvs);
        Uri expectedUri = passwordUri.buildUpon().appendPath("1").build();
        mAsserter.is(uri.toString(), expectedUri.toString(), "Insert returned correct uri");

        Cursor list = mActions.querySql(dbPath, "SELECT encryptedUsername FROM moz_logins");
        list.moveToFirst();
        String decryptedU = null;
        try {
            decryptedU = NSSBridge.decrypt(context, mProfile, list.getString(0));
        } catch (Exception e) {
            mAsserter.ok(false, "NSSBridge.decrypt through Exception " + e, ""); // TODO: What is diag?
        }
        mAsserter.is(decryptedU, "username", "Username was encrypted correctly when inserting");

        list = mActions.querySql(dbPath, "SELECT encryptedPassword, encType FROM moz_logins");
        list.moveToFirst();
        String decryptedP = null;
        try {
            decryptedP = NSSBridge.decrypt(context, mProfile, list.getString(0));
        } catch (Exception e) {
            mAsserter.ok(false, "NSSBridge.decrypt through Exception " + e, ""); // TODO: What is diag?
        }
        mAsserter.is(decryptedP, "password", "Password was encrypted correctly when inserting");
        mAsserter.is(list.getInt(1), 1, "Password has correct encryption type");

        cvs.put("encryptedUsername", "username2");
        cvs.put("encryptedPassword", "password2");
        cr.update(passwordUri, cvs, null, null);

        list = mActions.querySql(dbPath, "SELECT encryptedUsername FROM moz_logins");
        list.moveToFirst();
        try {
            decryptedU = NSSBridge.decrypt(context, mProfile, list.getString(0));
        } catch (Exception e) {
            mAsserter.ok(false, "NSSBridge.decrypt through Exception " + e, ""); // TODO: What is diag?
        }
        mAsserter.is(decryptedU, "username2", "Username was encrypted when updating");

        list = mActions.querySql(dbPath, "SELECT encryptedPassword FROM moz_logins");
        list.moveToFirst();
        try {
            decryptedP = NSSBridge.decrypt(context, mProfile, list.getString(0));
        } catch (Exception e) {
            mAsserter.ok(false, "NSSBridge.decrypt through Exception " + e, ""); // TODO: What is diag?
        }
        mAsserter.is(decryptedP, "password2", "Password was encrypted when updating");

        // Trying to store a password while master password is enabled should throw,
        // but because Android can't send Exceptions across processes
        // it just results in a null uri/cursor being returned.
        toggleMasterPassword("password");
        try {
            uri = cr.insert(passwordUri, cvs);
            // TODO: restore this assertion -- see bug 764901
            // mAsserter.is(uri, null, "Storing a password while MP was set should fail");

            Cursor c = cr.query(passwordUri, null, null, null, null);
            // TODO: restore this assertion -- see bug 764901
            // mAsserter.is(c, null, "Querying passwords while MP was set should fail");
        } catch (Exception ex) {
            // Password provider currently can not throw across process
            // so we should not catch this exception here
            mAsserter.ok(false, "Caught exception", ex.toString());
        }
        toggleMasterPassword("password");
    }

    private void toggleMasterPassword(String passwd) {
        JSONObject jsonPref = new JSONObject();
        try {
            jsonPref.put("name", "privacy.masterpassword.enabled");
            jsonPref.put("type", "string");
            jsonPref.put("value", passwd);
            mActions.sendGeckoEvent("Preferences:Set", jsonPref.toString());

            // Wait for confirmation of the pref change before proceeding with the test.
            final String[] prefNames = { "privacy.masterpassword.enabled" };
            final int ourRequestId = 0x73577;
            Actions.RepeatedEventExpecter eventExpecter = mActions.expectGeckoEvent("Preferences:Data");
            mActions.sendPreferencesGetEvent(ourRequestId, prefNames);

            JSONObject data = null;
            int requestId = -1;

            // Wait until we get the correct "Preferences:Data" event
            while (requestId != ourRequestId) {
                data = new JSONObject(eventExpecter.blockForEventData());
                requestId = data.getInt("requestId");
            }
        } catch (Exception ex) { 
            mAsserter.ok(false, "exception in toggleMasterPassword", ex.toString());
        }
    }

    @Override
    public void tearDown() throws Exception {
        // remove the entire signons.sqlite file
        File profile = new File(mProfile);
        File db = new File(profile, "signons.sqlite");
        if (db.delete()) {
            mAsserter.dumpLog("tearDown deleted "+db.toString());
        } else {
            mAsserter.dumpLog("tearDown did not delete "+db.toString());
        }

        super.tearDown();
    }
}
