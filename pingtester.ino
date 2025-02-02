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
// Making a gift for George and Kalan
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

Adafruit_AlphaNum4 alpha4_1 = Adafruit_AlphaNum4();
Adafruit_AlphaNum4 alpha4_0 = Adafruit_AlphaNum4();


// Replace with default credentials if desired, 
const char *DEFAULT_SSID = "MyWiFi";
const char *DEFAULT_PASSWORD = "MyPassword";

// Timeout for connecting to Wi-Fi
const unsigned long WIFI_TIMEOUT_MS = 20000;



// Host to ping
// const char *DESTINATION = "www.google.com"; // Example: Google's public DNS

Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

bool isConnected = false;


// global output string to have consistency across runs
String outputText = "<------>";

// how many times since I showed the site I'm pinging

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




// here's the form for configuring the WiFi network and the destination to ping
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
    // straight from ChatGPT, baby.

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
void displayStringAcrossTwoDisplays(String text) {

  // Serial.printf("text length = %d\n", text.length());

  // return;

  // add spaces to the end so we don't get null
  // yes, I know this is a terrible hack, and it shouldn't happen, 
  text += "        ";

  

  // String paddedText = padString(text, 8);

  // Split the text for the two displays
  //String alpha4_0_text = paddedText.substring(0, 3);
  //String alpha4_1_text = paddedText.substring(4, 8);

  // Clear both displays
  alpha4_0.clear();
  alpha4_1.clear();

  // Write to Display 1
  for (int i = 0; i <= 3; i++) {
    char c = text.charAt(i);
    //Serial.printf("Display 1: %u %c \n", i, c);
    alpha4_0.writeDigitAscii(i, c); // Write each character to the display
  }

  // Write to Display 2
  for (int i = 0; i<= 3; i++) {
    char c = text.charAt(i+4); // remember we're showing the next 4 digits
    alpha4_1.writeDigitAscii(i, c); // Write each character to the display
  }

  // Update both displays
  alpha4_0.writeDisplay();
  alpha4_1.writeDisplay();
}

void scrollText(String text, int displayWidth, int delayTime) {
    String paddedText = "        " + text + "        ";  // Pad with spaces for smooth scrolling
    int textLength = paddedText.length();

    for (int i = 0; i <= textLength - displayWidth; i++) {
        String frame = paddedText.substring(i, i + displayWidth);
        displayStringAcrossTwoDisplays(frame); 
        delay(delayTime);
    }
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

  
  // setup the LED displays 

  Wire.begin(SDA, SCL);  // SDA pin 9 and one in on LCD board, SLC pin 18 and rightmost on LCD board 
  
  // setup the LED displays by device number
  alpha4_0.begin(0x70);  // first one
  alpha4_1.begin(0x71);  // 2nd one
  
  alpha4_0.clear();
  alpha4_1.clear();

//  alpha4_0.writeDigitAscii(0,50);
//  alpha4_1.writeDigitAscii(0,51);

  // // Update both displays
  // alpha4_0.writeDisplay();
  // alpha4_1.writeDisplay();

//  displayStringAcrossTwoDisplays("*Setup*");

  scrollText("Ping Toy by andy.maxwell@workday.com", 8, 200);


  // get the stored Wifi credentials
  String ssid = preferences.getString("ssid", DEFAULT_SSID);
  String password = preferences.getString("password", DEFAULT_PASSWORD);


  // try to connect to WiFi using stored creds
  if (connectToWiFi(ssid.c_str(), password.c_str())) {
    isConnected = true;
    startServer();  // used to setup the destination to ping
  } else {
    startAccessPoint(); // used ot setup the WiFi connection and destination to ping
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
    scrollText("Connect to Access Point: PingToy, password LoganMcNeil", 8, 200);
  
  WiFi.softAP(apSSID, apPassword);
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("AP started. IP: %s\n", IP.toString().c_str());

  char tempOut[20];
  sprintf(tempOut, "IP: %s\n", IP.toString());

  for (int i = 0; i < 3; i++)
    scrollText(tempOut, 8, 200);

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

      server.send(200, "text/html", "<h1>Credentials Saved. Restarting.</h1>"); // add comment for IP saved
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

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPageDest);
  });

  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("pingDest")) {

      preferences.putString("pingDest", server.arg("pingDest"));


      server.send(200, "text/html", "<h1>Destination Saved. Restarting.</h1>"); // add comment for IP saved
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/html", "<h1>Invalid Input</h1><p><a href =\"\\\">home</a>");
    }
  });

  server.begin();
  
}



void loop() {
  
  //  String outputText = "<------>"; // MADE GLOBAL

  char outputChar[20] = "xxxxxxx";

  // displayStringAcrossTwoDisplays(outputText);


  if (isConnected) {
  
    Serial.println("Doing work while connected to Wi-Fi...");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    server.handleClient();

    // displayStringAcrossTwoDisplays("Connected");


    String pingDestStr = preferences.getString("pingDest", "www.workday.com");

    const char* remote_host = pingDestStr.c_str();

    // TODO: don't show it every time. Show it just once in a while
    if (siteShowCount >= siteShowCountMax) {
      scrollText(pingDestStr, 8, 150);
      siteShowCount = 0;
    } else {
      siteShowCount++;
    }

//    Serial.println(siteShowCount);

    displayStringAcrossTwoDisplays(outputText); // show the last output output text while running the next ping
  

    Serial.print(remote_host);
    if (Ping.ping(remote_host) > 0){
      Serial.printf(" response time : %d/%.2f/%d ms\n", Ping.minTime(), Ping.averageTime(), Ping.maxTime());
      sprintf(outputChar, "%.2f", Ping.averageTime());
      padString(outputChar, 8);
      outputText = outputChar;

    } else {
      Serial.println(" Ping Error !");
      outputText = "**Error**";
    }
    
    displayStringAcrossTwoDisplays(outputText);
    
    delay(2000); 

    // format for display on LED or servo.
  } else {
    // In case you want to handle something when not connected
    // write error to LED
    Serial.println("Not connected to Wi-Fi.");
    displayStringAcrossTwoDisplays("No Wifi");
    delay(1000);
  }
}
