// ######                          #######
// #     #  #  #    #   ####          #     ######   ####   #####  ######  #####
// #     #  #  ##   #  #    #         #     #       #         #    #       #    #
// ######   #  # #  #  #              #     #####    ####     #    #####   #    #
// #        #  #  # #  #  ###         #     #            #    #    #       #####
// #        #  #   ##  #    #         #     #       #    #    #    #       #   #
// #        #  #    #   ####          #     ######   ####     #    ######  #    #
//
// Ping Tester
// Andy Maxwell | andy@maxwell.nyc
// 12/4/2024
// Making a gift for some folks around Workday like Sam and Kalan and George
// for the ESP32-S2 Dev Board,
// use the COM port, not USB

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPping.h>

// LED output
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// instantiate the two i2c LED controllers
Adafruit_AlphaNum4 alpha4_1 = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 alpha4_0 = Adafruit_AlphaNum4();

const int displayLength = 8;


// Replace with default credentials if desired,
// don't need to since it'll spin up an AP and website for you to configure it.
const char *DEFAULT_SSID = "MyWiFi";
const char *DEFAULT_PASSWORD = "MyPassword";

// Timeout for connecting to Wi-Fi
const unsigned long WIFI_TIMEOUT_MS = 20000;

// store settings between sessions
Preferences preferences;

// stand up the web server so we can config the settings
WebServer server(80);
DNSServer dnsServer;

bool isConnected = false;  // global variable to show WiFi state


// global output string to have consistency across runs
String outputText = "<------>";  // the output to show on the display, preseve between loops.

int dpAt = -1;  // position of the decimal point.  -1 to turn off

// how many times since I showed the site address I'm pinging
// it'll show the machine it's pinging that number of pings.
const int siteShowCountMax = 30;
int siteShowCount = siteShowCountMax;


// here's the form for configuring the WiFi network and the destination to ping
String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Pinger Toy Config</title>
</head>
<body>
  <h1>Andy's Pinger Toy Config</h1>
   <h2>Configure WiFi</h2>
  <form action="/save" method="post">
    <label for="ssid">SSID:</label><br>
    <input type="text" id="ssid" name="ssid"><br><br>
    <label for="password">Password:</label><br>
    <input type="password" id="password" name="password"><br><br>
    <label for="pingDest">IP to ping:</label><br>
    <input type="text" id="pingDest" name="pingDest"><br><br>
    <input type="submit" value="Save">
  </form>
</body>
</html>
)rawliteral";


// here's the form for configuring the WiFi network and the destination to ping when on WiFi.
String htmlPageDest = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Pinger Toy Config</title>
</head>
<body>
  <h1>Andy's Pinger Toy Config</h1>
   <h2>Configure Destination to Ping</h2>
  <form action="/save" method="post">
    <label for="pingDest">Server to ping:</label><br>
    <input type="text" id="pingDest" name="pingDest"><br><br>
    <input type="submit" value="Save">
  </form>
</body>
</html>
)rawliteral";


void padString(char *str, int maxLength) {
  // straight from ChatGPT, baby.  It's a good Jr Programmer.

  int len = strlen(str);

  // If the string is longer than maxLength, truncate it
  if (len > maxLength) {
    str[maxLength] = '\0';  // Cut off at maxLength
    return;
  }

  // If shorter, pad with spaces
  int padSize = maxLength - len;
  char temp[maxLength + 1];  // Temporary buffer

  // Fill with spaces
  memset(temp, ' ', padSize);

  // Copy original string to the end of the temp buffer
  strcpy(temp + padSize, str);

  // Copy back to original string
  strncpy(str, temp, maxLength);
  str[maxLength] = '\0';  // Ensure null termination
}

//  ____  _           _
// |  _ \(_)___ _ __ | | __ _ _   _
// | | | | / __| '_ \| |/ _` | | | |
// | |_| | \__ \ |_) | | (_| | |_| |
// |____/|_|___/ .__/|_|\__,_|\__, |
//             |_|            |___/


// Function to display a string across two displays
void displayStringAcrossTwoDisplays(String text, int dPLocation = -1) {

  // add spaces to the end so we don't get null
  // yes, I know this is a terrible hack, and it shouldn't happen,
  text += "        ";



  // Clear both displays
  alpha4_0.clear();
  alpha4_1.clear();


  // Write to Display 1
  // you can only set one character at one position at a time
  // and there's 4 characters per display
  // so go through the first 4 characters of the text, put them in the spots
  // and if you're on the character where the decimal point is, turn on the bool
  // It's weird, but that's because there's no ASCII modifier meaning "number or letter with a decimal point"
  for (int i = 0; i <= 3; i++) {
    char c = text.charAt(i);
    alpha4_0.writeDigitAscii(i, c, i == dPLocation);  // Write each character to the display, if it's the character with the decimal point, show it
  }

  // Write to Display 2
  for (int i = 0; i <= 3; i++) {
    char c = text.charAt(i + 4);                            // remember we're showing the next 4 digits
    alpha4_1.writeDigitAscii(i, c, (i == dPLocation - 4));  // Write each character to the display, ditto for the decimal point
  }

  // Update both displays
  alpha4_0.writeDisplay();
  alpha4_1.writeDisplay();
}



void scrollText(String text, int displayWidth, int delayTime) {
  String paddedText = "        " + text + "        ";  // Pad with spaces front and back for smooth intro and exit
  int textLength = paddedText.length();

  for (int i = 0; i <= textLength - displayWidth; i++) {
    String frame = paddedText.substring(i, i + displayWidth);
    displayStringAcrossTwoDisplays(frame);
    delay(delayTime);
  }
}


void blink(bool blinkOn) {

  if (blinkOn) {
    alpha4_0.blinkRate(HT16K33_BLINK_2HZ);
    alpha4_1.blinkRate(HT16K33_BLINK_2HZ);
  } else {
    alpha4_0.blinkRate(HT16K33_BLINK_OFF);
    alpha4_1.blinkRate(HT16K33_BLINK_OFF);
  }
}


void egg() {
  // easter egg

  // pop up once every hour or so
  long tempRand = random(1800);

  // TODO, add a button to turn on to make it happen

  //  displayStringAcrossTwoDisplays(String(tempRand));
  //  delay(200);
  if (tempRand != 1) {
    return;
  }

  // you hear the bell, you get a prize!

  const String messages[] = {
    "Hello, world.",
    "DATA IS LIKE PEE IN THE POOL",
    "IT'S ALWAYS DNS",
    "Workday... AND NIGHT",
    "EVERYTHING IS AN OBJECT",
    "ASSETS = LIABILITIES + EQUITY",
    "DEBUGGER?? I USE PRINT",
    "It works on my machine.",
    "Pleasanton Marriott is better than the AC.",
    "Don't take BART from SFO.",
    "LGA, JFA, EWR, LAS, SFO, MIA, PDX, DFW, MCO, ORD",
    "I'm afraid I can't do that, Dave.",
    "Are you Sarah Connor?",
    "I'VE GOT 1099 PROBLEMS, BUT A W-2 AIN'T ONE",
    "V = I x R",
    "F = M x A",
    "sudo make me a sandwich",
    "The bathroom code is 1234*",
    "sudo rm -fr /",
    "[object Object]",
    "There's no place like 127.0.0.1",
    "AI stole my job.",
    "Recursion. See recursion.",
    "Error 418: I'm a teapot.",
    "There are 10 types of people.",
    "Bugs everywhere.",
    "0xDEADBEEF",
    "1/0",
    "NaN",
    "#REF",
    "An Error occurred when processing the query. The query returned too many results.. Cause: com.workday.exceptions.EndUserException: An Error occurred when processing the query. The query returned too many results."
  };

  // Arrays of strings areweird in C so yeah, I'm hard coding the length
  // just be glad I didn't use the weird Harvard Architecture PROGMEM
  const int numMessages = 31;
  displayStringAcrossTwoDisplays("********");
  delay(1000);
  displayStringAcrossTwoDisplays("- ANDY -");
  delay(2000);
  displayStringAcrossTwoDisplays("- SAYS -");
  delay(2000);

  scrollText(messages[random(0, numMessages - 1)], 8, 200);
  displayStringAcrossTwoDisplays("********");
  delay(1000);
}



//  ____       _
// / ___|  ___| |_ _   _ _ __
// \___ \ / _ \ __| | | | '_ \ 
//  ___) |  __/ |_| |_| | |_) |
// |____/ \___|\__|\__,_| .__/
//                      |_|


void setup() {
  Serial.begin(9600);
  preferences.begin("wifi-creds", false);


  Serial.print("in Setup\n");
  Serial.print("Source at:\nhttps://github.com/andyhomecode/pingtester\n");

  randomSeed(analogRead(0));

  // setup the LED displays

  Wire.begin(SDA, SCL);  // SDA pin 9 and one in on LCD board, SLC pin 18 and rightmost on LCD board

  // setup the LED displays by device number
  alpha4_0.begin(0x70);  // first one
  alpha4_1.begin(0x71);  // 2nd one

  alpha4_0.clear();
  alpha4_1.clear();

  // title screen
  scrollText("Ping Toy by andy.maxwell@workday.com", displayLength, 200);

  // get the stored Wifi credentials
  String ssid = preferences.getString("ssid", DEFAULT_SSID);
  String password = preferences.getString("password", DEFAULT_PASSWORD);


  // try to connect to WiFi using stored creds
  if (connectToWiFi(ssid.c_str(), password.c_str())) {
    isConnected = true;
    startServer();  // used to setup the destination to ping
  } else {
    startAccessPoint();  // used ot setup the WiFi connection and destination to ping
  }
}

bool connectToWiFi(const char *ssid, const char *password) {
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to WiFi: %s\n", ssid);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected to %s\n", ssid);
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    return true;
  } else {
    Serial.println("\nFailed to connect.");
    return false;
  }
}

void startAccessPoint() {
  const char *apSSID = "PingToy";
  const char *apPassword = "LoganMcNeil";


  for (int i = 0; i < 3; i++)
    scrollText("Connect to Access Point: PingToy, password LoganMcNeil", displayLength, 200);

  WiFi.softAP(apSSID, apPassword);
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("AP started. IP: %s\n", IP.toString().c_str());

  char tempOut[20];
  sprintf(tempOut, "IP: %s\n", IP.toString());

  for (int i = 0; i < 3; i++)
    scrollText(tempOut, displayLength, 200);

  dnsServer.start(53, "*", IP);


  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
      // String ssid = server.arg("ssid");
      // String password = server.arg("password");

      preferences.putString("ssid", server.arg("ssid"));
      preferences.putString("password", server.arg("password"));
      preferences.putString("pingDest", server.arg("pingDest"));

      server.send(200, "text/html", "<h1>Credentials Saved. Restarting.</h1>");  // add comment for IP saved
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/html", "<h1>Invalid Input</h1>");
    }
  });

  server.begin();
  while (true) {
    dnsServer.processNextRequest();
    server.handleClient();
  }
}

void startServer() {

  // this is the server that's running when the server is connected to Wifi
  // it's not a thread, so it needs to be called to process.

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPageDest);
  });

  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("pingDest")) {

      preferences.putString("pingDest", server.arg("pingDest"));


      server.send(200, "text/html", "<h1>Destination Saved. Restarting.</h1>");  // add comment for IP saved
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/html", "<h1>Invalid Input</h1><p><a href =\"\\\">home</a>");
    }
  });

  server.begin();
}



void loop() {

  char outputChar[20] = "xxxxxxx";  // local temp output for the loop

  if (isConnected) {
    // we're on wifi.  good deal
    Serial.println("Doing work while connected to Wi-Fi...");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    server.handleClient();  // let the Web Server run to handle things like saving new address to ping.

    String pingDestStr = preferences.getString("pingDest", "www.google.com");

    const char *remote_host = pingDestStr.c_str();

    // show what site we're pinging to every now and then
    if (siteShowCount >= siteShowCountMax) {
      scrollText(pingDestStr, displayLength, 150);
      siteShowCount = 0;
    } else {
      siteShowCount++;
    }

    displayStringAcrossTwoDisplays(outputText, dpAt);  // show the last output output text while running the next ping


    // THIS IS THE MEAT RIGHT HERE
    // PING IT, FORMAT IT, SHOW IT.

    Serial.print(remote_host);
    if (Ping.ping(remote_host) > 0) {
      Serial.printf(" response time : %d/%.1f/%d ms\n", Ping.minTime(), Ping.averageTime(), Ping.maxTime());
      sprintf(outputChar, "%.0fms", Ping.averageTime() * 10);  // format it so 12.3ms looks like that, but without the decimal point
      dpAt = 4;                                                // turn on the decimal point, put it where it should go in the format above
      padString(outputChar, 8);                                // left pad it because it's a number. (and no, I didn't use the NPM package)
      outputText = outputChar;                                 // put it in the global so it'll survive the loop and can be reshown until the next ping.
      blink(Ping.averageTime() > 150);                         // if the ping is slow, turn on blink. TODO: Make it a web setting


    } else {
      // oops.
      blink(true);
      Serial.println(" Ping Error !");
      outputText = "**Error**";
      dpAt = -1;  // turn of the decimal point
    }

    // show the output
    displayStringAcrossTwoDisplays(outputText, dpAt);

    // let them see it.
    delay(2000);

    blink(false);
    egg();

    // format for display on LED or servo.
  } else {
    // In case you want to handle something when not connected
    Serial.println("Not connected to Wi-Fi.");
    displayStringAcrossTwoDisplays("No Wifi");
    delay(1000);
  }
}
