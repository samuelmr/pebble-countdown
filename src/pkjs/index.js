var initialized = false;
var messageQueue = [];

function sendNextMessage() {
  if (messageQueue.length > 0) {
    Pebble.sendAppMessage(messageQueue[0], appMessageAck, appMessageNack);
    console.log("Sent message to Pebble! " + JSON.stringify(messageQueue[0]));
  }
}

function appMessageAck(e) {
  console.log("Message accepted by Pebble!");
  messageQueue.shift();
  sendNextMessage();
}

function appMessageNack(e) {
  // console.log("options not sent to Pebble: " + e.error.message);
  console.log("Message rejected by Pebble! " + e.error);
}

Pebble.addEventListener("ready",
  function(e) {
    console.log("JavaScript app ready and running!");
    initialized = true;
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    var options = JSON.parse(decodeURIComponent(e.response));
    console.log("Webview window returned: " + JSON.stringify(options));
    var time = options["0"].split(":");
    var hours = 0;
    if (time.length == 3) {
     hours = parseInt(time[0]);
    }
    var minutes = parseInt(time[time.length-2]);
    var seconds = parseInt(time[time.length-1]);
    var timeopts = {'hours': hours,
                    'minutes': minutes,
                    'seconds': seconds,
                    'long': options["1"],
                    'single': options["2"],
                    'double': options["3"]};
    console.log("options formed: " + JSON.stringify(timeopts));
    messageQueue.push(timeopts);
    sendNextMessage();
  }
);

Pebble.addEventListener("showConfiguration",
  function() {
    var uri = "https://samuelmr.github.io/pebble-countdown/configure.html";
    console.log("Configuration url: " + uri);
    Pebble.openURL(uri);
  }
);
