/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* This module is imported at the startup of an application.  It takes care of
 * permissions installation, application url loading, security settings.  Put
 * stuff here that you want to happen once on startup before the webapp is
 * loaded.  */

this.EXPORTED_SYMBOLS = ["startup"];

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
// Initialize DOMApplicationRegistry by importing Webapps.jsm.
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionsInstaller.jsm");
Cu.import('resource://gre/modules/Payment.jsm');
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

// Initialize window-independent handling of webapps- notifications.
Cu.import("resource://webapprt/modules/WebappsHandler.jsm");
Cu.import("resource://webapprt/modules/WebappRT.jsm");
Cu.import("resource://webapprt/modules/WebRTCHandler.jsm");

const PROFILE_DIR = OS.Constants.Path.profileDir;

function isFirstRunOrUpdate() {
  let savedBuildID = null;
  try {
    savedBuildID = Services.prefs.getCharPref("webapprt.buildID");
  } catch (e) {}

  let ourBuildID = Services.appinfo.platformBuildID;

  if (ourBuildID != savedBuildID) {
    Services.prefs.setCharPref("webapprt.buildID", ourBuildID);
    return true;
  }

  return false;
}

function writeFile(aPath, aData) {
  return Task.spawn(function() {
    let data = TextEncoder().encode(aData);
    yield OS.File.writeAtomic(aPath, data, { tmpPath: aPath + ".tmp" });
  });
}

function createBrandingFiles() {
  return Task.spawn(function() {
    let manifest = WebappRT.localeManifest;
    let name = WebappRT.localeManifest.name;
    let developer = " ";
    if (WebappRT.localeManifest.developer) {
      developer = WebappRT.localeManifest.developer.name;
    }

    let brandDTDContent = '<!ENTITY brandShortName "' + name + '">\n\
  <!ENTITY brandFullName "' + name + '">\n\
  <!ENTITY vendorShortName "' + developer + '">\n\
  <!ENTITY trademarkInfo.part1 " ">';

    yield writeFile(OS.Path.join(PROFILE_DIR, "brand.dtd"), brandDTDContent);

    let brandPropertiesContent = 'brandShortName=' + name + '\n\
  brandFullName=' + name + '\n\
  vendorShortName=' + developer;

    yield writeFile(OS.Path.join(PROFILE_DIR, "brand.properties"),
                    brandPropertiesContent);
  });
}

// Observes all the events needed to actually launch an application.
// It waits for XUL window and webapps registry loading.
this.startup = function(window) {
  return Task.spawn(function () {
    // Observe registry loading.
    let deferredRegistry = Promise.defer();
    function observeRegistryLoading() {
      Services.obs.removeObserver(observeRegistryLoading, "webapps-registry-start");
      deferredRegistry.resolve();
    }
    Services.obs.addObserver(observeRegistryLoading, "webapps-registry-start", false);

    // Observe XUL window loading.
    // For tests, it could be already loaded.
    let deferredWindowLoad = Promise.defer();
    if (window.document && window.document.getElementById("content")) {
      deferredWindowLoad.resolve();
    } else {
      window.addEventListener("load", function onLoad() {
        window.removeEventListener("load", onLoad, false);
        deferredWindowLoad.resolve();
      });
    }

    // Wait for webapps registry loading.
    yield deferredRegistry.promise;

    // Install/update permissions and get the appID from the webapps registry.
    let appID = Ci.nsIScriptSecurityManager.NO_APP_ID;
    let manifestURL = WebappRT.config.app.manifestURL;
    if (manifestURL) {
      appID = DOMApplicationRegistry.getAppLocalIdByManifestURL(manifestURL);

      // On firstrun, set permissions to their default values.
      // When the webapp runtime is updated, update the permissions.
      // TODO: Update the permissions when the application is updated.
      if (isFirstRunOrUpdate(Services.prefs)) {
        PermissionsInstaller.installPermissions(WebappRT.config.app, true);
        yield createBrandingFiles();
      }
    }

    // Branding substitution
    let aliasFile = Components.classes["@mozilla.org/file/local;1"]
                              .createInstance(Ci.nsIFile);
    aliasFile.initWithPath(PROFILE_DIR);

    let aliasURI = Services.io.newFileURI(aliasFile);

    Services.io.getProtocolHandler("resource")
               .QueryInterface(Ci.nsIResProtocolHandler)
               .setSubstitution("webappbranding", aliasURI);

    // Wait for XUL window loading
    yield deferredWindowLoad.promise;

    // Get the <browser> element in the webapp.xul window.
    let appBrowser = window.document.getElementById("content");

    // Set the principal to the correct appID and launch the application.
    appBrowser.docShell.setIsApp(appID);
    appBrowser.setAttribute("src", WebappRT.launchURI);
  }).then(null, Cu.reportError.bind(Cu));
}
