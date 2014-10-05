dump('loaded child cpow test\n');

content.document.title = "Hello, Kitty";

(function start() {
    sync_test();
    async_test();
    rpc_test();
    nested_sync_test();
    // The sync-ness of this call is important, because otherwise
    // we tear down the child's document while we are
    // still in the async test in the parent.
    sendSyncMessage("cpows:done", {});
  }
)();

function ok(condition, message) {
  dump('condition: ' + condition  + ', ' + message + '\n');
  if (!condition) {
    sendAsyncMessage("cpows:fail", { message: message });
    throw 'failed check: ' + message;
  }
}

var sync_obj;
var async_obj;

function make_object()
{
  let o = { };
  o.i = 5;
  o.b = true;
  o.s = "hello";
  o.x = { i: 10 };
  o.f = function () { return 99; };

  // Doing anything with this Proxy will throw.
  var throwing = new Proxy({}, new Proxy({}, {
      get: function (trap) { throw trap; }
    }));

  let array = [1, 2, 3];

  let for_json = { "n": 3, "a": array, "s": "hello", o: { "x": 10 } };

  return { "data": o,
           "throwing": throwing,
           "document": content.document,
           "array": array,
           "for_json": for_json
         };
}

function make_json()
{
  return { check: "ok" };
}

function sync_test()
{
  dump('beginning cpow sync test\n');
  sync_obj = make_object();
  sendSyncMessage("cpows:sync",
    make_json(),
    make_object());
}

function async_test()
{
  dump('beginning cpow async test\n');
  async_obj = make_object();
  sendAsyncMessage("cpows:async",
    make_json(),
    async_obj);
}

function rpc_test()
{
  dump('beginning cpow rpc test\n');
  rpc_obj = make_object();
  rpc_obj.data.reenter = function  () {
    sendRpcMessage("cpows:reenter", { }, { data: { valid: true } });
    return "ok";
  }
  sendRpcMessage("cpows:rpc",
    make_json(),
    rpc_obj);
}

function nested_sync_test()
{
  dump('beginning cpow nested sync test\n');
  sync_obj = make_object();
  sync_obj.data.reenter = function () {
    let caught = false;
    try {
       sendSyncMessage("cpows:reenter_sync", { }, { });
    } catch (e) {
      caught = true;
    }
    if (!ok(caught, "should not allow nested sync"))
      return "fail";
    return "ok";
  }
  sendSyncMessage("cpows:nested_sync",
    make_json(),
    rpc_obj);
}
