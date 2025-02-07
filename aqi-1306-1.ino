

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Livebox-4650";
const char* password = "MaisonDANDAULT ";
const String apiKey = "761239f3ef70e2b00837a8b725224ce182e10c26";
const String apiEndpoint = "https://api.waqi.info/feed/here/?token=" + apiKey;

void setup() {
  Serial.begin(115200);
 

  delay(100); // give a little time for serial to start up

  Serial.println("Setup started");
  

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Initialize display 
  display.begin();
  
  Serial.println("Display initialized");

  Serial.println("Clearing display...");
  display.clearDisplay();
  
  Serial.println("Display cleared");
  Serial.println("Displaying...");
  display.display();

  Serial.println("Done!");
}

void loop() {
  
  Serial.print(". ");
  Serial.println("Sending HTTP request...");
  // Make the HTTP request
  WiFiClient client; // Use WiFiClient instead of Client
  HTTPClient http;
  http.begin(client, apiEndpoint);

  int httpCode = http.GET();
   Serial.println("HTTP response code: " + String(httpCode));
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      Serial.println("HTTP request successful");
      String payload = http.getString();
       Serial.println("Receiving response...");

      // Parse JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      // Extract data
      String location = doc["data"]["city"]["name"];
      String date = doc["data"]["time"]["s"];
      int aqi = doc["data"]["aqi"];
      
      Serial.println("Extracting data...");
      
      Serial.println("Location: " + location);
      Serial.println("Date: " + date);
      Serial.println("AQI: " + String(aqi));


      // Display on OLED
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("Location: " + location);
      display.println("Date: " + date);
      display.println("AQI: " + String(aqi));

      display.display();
    }
  } else {
    Serial.println("HTTP request failed");
  }

  http.end();

  delay(30000); // Delay for 30 seconds before the next request
}
