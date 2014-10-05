/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

MARIONETTE_TIMEOUT = 60000;

SpecialPowers.setBoolPref("dom.sms.enabled", true);
SpecialPowers.addPermission("sms", true, document);

let manager = window.navigator.mozMobileMessage;
let myNumber = "15555215554";
let inText = "Incoming SMS message. Mozilla Firefox OS!";
let remoteNumber = "5559997777";
let inSmsId = 0;

function verifyInitialState() {
  log("Verifying initial state.");
  ok(manager instanceof MozMobileMessageManager,
     "manager is instance of " + manager.constructor);
  simulateIncomingSms();  
}

function simulateIncomingSms() {
  log("Simulating incoming SMS.");

  manager.onreceived = function onreceived(event) {
    log("Received 'onreceived' event.");
    let incomingSms = event.message;
    ok(incomingSms, "incoming sms");
    ok(incomingSms.id, "sms id");
    inSmsId = incomingSms.id;
    log("Received SMS (id: " + inSmsId + ").");
    is(incomingSms.body, inText, "msg body");
    is(incomingSms.delivery, "received", "delivery");
    getNonExistentMsg();
  };
  // Simulate incoming sms sent from remoteNumber to our emulator
  runEmulatorCmd("sms send " + remoteNumber + " " + inText, function(result) {
    is(result[0], "OK", "emulator output");
  });
}

function getNonExistentMsg() {
  let msgIdNoExist = inSmsId + 1;
  log("Attempting to get non-existent message (id: " + msgIdNoExist + ").");
  let requestRet = manager.getMessage(msgIdNoExist);
  ok(requestRet, "smsrequest obj returned");

  requestRet.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    ok(event.target.result, "smsrequest event.target.result");
    let foundSms = event.target.result;
    log("Got SMS (id: " + foundSms.id + ") but should not have.");
    ok(false, "Smsrequest successful when tried to get non-existent sms");
    getMsgInvalidId();
  };

  requestRet.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    is(event.target.error.name, "NotFoundError", "error returned");
    log("Could not get SMS (id: " + msgIdNoExist + ") as expected.");
    getMsgInvalidId();
  };
}  

function getMsgInvalidId() {
  invalidId = -1;
  log("Attempting to get sms with invalid id (id: " + invalidId + ").");
  let requestRet = manager.getMessage(invalidId);
  ok(requestRet, "smsrequest obj returned");

  requestRet.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    ok(event.target.result, "smsrequest event.target.result");
    let foundSms = event.target.result;
    log("Got SMS (id: " + foundSms.id + ") but should not have.");
    ok(false, "Smsrequest successful when tried to get message with " +
    		"invalid id (id: " + invalidId + ").");
    deleteMsg();
  };

  requestRet.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    is(event.target.error.name, "NotFoundError", "error returned");
    log("Could not get SMS (id: -1) as expected.");
    deleteMsg();
  };
}

function deleteMsg() {
  log("Deleting SMS (id: " + inSmsId + ").");
  let requestRet = manager.delete(inSmsId);
  ok(requestRet,"smsrequest obj returned");

  requestRet.onsuccess = function(event) {
    log("Received 'onsuccess' smsrequest event.");
    if(event.target.result){
      cleanUp();
    }
  };

  requestRet.onerror = function(event) {
    log("Received 'onerror' smsrequest event.");
    ok(event.target.error, "domerror obj");
    ok(false, "manager.delete request returned unexpected error: "
        + event.target.error.name );
    cleanUp();
  };
}

function cleanUp() {
  manager.onreceived = null;
  SpecialPowers.removePermission("sms", document);
  SpecialPowers.setBoolPref("dom.sms.enabled", false);
  finish();
}

// Start the test
verifyInitialState();
