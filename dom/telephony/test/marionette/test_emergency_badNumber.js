/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

let number = "not a valid emergency number";
let outgoing;
let calls;

function dial() {
  log("Make an outgoing call to an invalid number.");

  outgoing = telephony.dialEmergency(number);
  ok(outgoing);
  is(outgoing.number, number);
  is(outgoing.state, "dialing");

  is(outgoing, telephony.active);
  //ok(telephony.calls === calls); // bug 717414
  is(telephony.calls.length, 1);
  is(telephony.calls[0], outgoing);

  outgoing.onerror = function onerror(event) {
    log("Received 'error' event.");
    is(event.call, outgoing);
    ok(event.call.error);
    is(event.call.error.name, "BadNumberError");

    is(telephony.calls.length, 0);
    is(telephony.active, null);

    emulator.run("gsm list", function(result) {
      log("Initial call list: " + result);
      is(result[0], "OK");
      cleanUp();
    });
  };
}

function cleanUp() {
  finish();
}

startTest(function() {
  dial();
});
