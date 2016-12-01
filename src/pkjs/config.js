Pebble.addEventListener("ready",
  function(e) {
    console.log("PebbleKit JS ready!");
  }
);
Pebble.addEventListener("showConfiguration",
  function(e) {
    //Load the remote config page
    Pebble.openURL("https://dl.dropboxusercontent.com/u/133899570/ACNL%20Config/config.html");
  }
);
Pebble.addEventListener("webviewclosed",
  function(e) {
    //Get JSON dictionary
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log("Configuration window returned: " + JSON.stringify(configuration));
 
    //Send to Pebble, persist there
    Pebble.sendAppMessage(
      {"KEY_BG": configuration.face},
      function(e) {
        console.log("Sending settings data...");
      },
      function(e) {
        console.log("Settings feedback failed!");
      }
    );
  }
);