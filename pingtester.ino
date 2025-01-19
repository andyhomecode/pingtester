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

// char displaybuffer[8] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};


#define LED_PIN 13 // GPIO pin for the built-in LED on ESP32-S3-WROOM-1



// Replace with default credentials if desired
const char *DEFAULT_SSID = "MyWiFi";
const char *DEFAULT_PASSWORD = "MyPassword";


// Host to ping
// const char *DESTINATION = "www.google.com"; // Example: Google's public DNS

Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

bool isConnected = false;

String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 WiFi Config</title>
</head>
<body>
  <h1>Andy's Pinger Toy</h1>
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
)rawliteral"; // need to add input for site to ping. 

// Timeout for connecting to Wi-Fi
const unsigned long WIFI_TIMEOUT_MS = 20000;


void setup() {
  Serial.begin(115200);
  preferences.begin("wifi-creds", false);

  // setup the LED display
  alpha4_0.begin(0x70);  // first one
  alpha4_1.begin(0x71);  // 2nd one
  
  pinMode(LED_PIN, OUTPUT); // Set LED pin as output

  digitalWrite(LED_PIN, HIGH); // Turn LED on
  delay(1000);                // Wait for 1 second
  digitalWrite(LED_PIN, LOW);  // Turn LED off
  delay(1000);                // Wait for 1 second


  String ssid = preferences.getString("ssid", DEFAULT_SSID);
  String password = preferences.getString("password", DEFAULT_PASSWORD);

  if (connectToWiFi(ssid.c_str(), password.c_str())) {
    isConnected = true;
    startServer();
  } else {
    startAccessPoint();
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
  const char *apSSID = "PingerToy";
  const char *apPassword = "LoganMcNeil";

  WiFi.softAP(apSSID, apPassword);
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("AP started. IP: %s\n", IP.toString().c_str());

  dnsServer.start(53, "*", IP);
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
      String ssid = server.arg("ssid");
      String password = server.arg("password");

      preferences.putString("ssid", ssid);
      preferences.putString("password", password);

      server.send(200, "text/html", "<h1>Credentials Saved. Restart the ESP32.</h1>"); // add comment for IP saved
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

   server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });

  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
      String ssid = server.arg("ssid");
      String password = server.arg("password");

      preferences.putString("ssid", ssid);
      preferences.putString("password", password);

      server.send(200, "text/html", "<h1>Credentials Saved. Restart the ESP32.</h1>"); // add comment for IP saved
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/html", "<h1>Invalid Input</h1>");
    }
  });

  server.begin();
  
}

String padString(String input, int totalLength) {
  while (input.length() < totalLength) {
    input += " "; // Append a space
  }
  return input;
}

// Function to display a string across two displays
void displayStringAcrossTwoDisplays(String text) {

  // add spaces to the end so we don't get null
  text += "        ";

  // String paddedText = padString(text, 8);

  // Split the text for the two displays
  //String alpha4_0_text = paddedText.substring(0, 3);
  //String alpha4_1_text = paddedText.substring(4, 8);

  // Clear both displays
  alpha4_0.clear();
  alpha4_1.clear();

  // Write to Display 1
  for (int i = 0; 3; i++) {
    char c = text.charAt(i);
    alpha4_0.writeDigitAscii(i, 'x'); // Write each character to the display
  }

  // Write to Display 2
  for (int i = 0; 3; i++) {
    char c = text.charAt(i+4); // remember we're showing the next 4 digits
    alpha4_1.writeDigitAscii(i, 'x'); // Write each character to the display
  }

  // Update both displays
  alpha4_0.writeDisplay();
  alpha4_1.writeDisplay();
}




void loop() {
  if (isConnected) {
    // Your main program logic goes here
    Serial.println("Doing work while connected to Wi-Fi...");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    server.handleClient();

    displayStringAcrossTwoDisplays("Andy1");

    const char* remote_host = "www.google.com";
    Serial.print(remote_host);
    if (Ping.ping(remote_host) > 0){
      Serial.printf(" response time : %d/%.2f/%d ms\n", Ping.minTime(), Ping.averageTime(), Ping.maxTime());
    } else {
      Serial.println(" Ping Error !");
    }    
    
    delay(1000); // Simulate work with a delay

    // format for display on LED or servo.
  } else {
    // In case you want to handle something when not connected
    // write error to LED
    Serial.println("Not connected to Wi-Fi.");
    delay(1000);
  }
}
