#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// For animation
#define ANIMATION_STEPS 10
#define ANIMATION_DELAY 20

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char *ssid = "your_wifi_ssid";
const char *password = "your_wifi_password";
const String apiKey = "your_api_key";
const String apiEndpoint = "https://api.waqi.info/feed/here/?token=" + apiKey;

// Free API for stocks (Alpha Vantage)
const char *alphaVantageNasdaq = "https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=QQQ&apikey=0KOOKHBB5VNTMZMM";
const char *alphaVantageCAC40 = "https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=CAC&apikey=0KOOKHBB5VNTMZMM";

// Store stock data
float nasdaqValue = 0;
float cac40Value = 0;
float prevNasdaq = 0;
float prevCAC40 = 0;
unsigned long lastStockUpdate = 0;
const unsigned long stockUpdateInterval = 300000; // 5 minutes

void animateNumber(int x, int y, int oldValue, int newValue, int textSize) {
    display.setTextSize(textSize);

    for (int step = 0; step <= ANIMATION_STEPS; step++) {
        float intermediateValue = oldValue + ((newValue - oldValue) * step / ANIMATION_STEPS);

        // Clear just the number area
        display.fillRect(x, y, textSize * 6 * 5, textSize * 8, BLACK); // Assuming max 5 digits
        display.setCursor(x, y);
        display.print((int)intermediateValue);
        display.display();
        delay(ANIMATION_DELAY);
    }
}

void animateFloat(int x, int y, float oldValue, float newValue, int textSize, int decimalPlaces) {
    display.setTextSize(textSize);

    for (int step = 0; step <= ANIMATION_STEPS; step++) {
        float intermediateValue = oldValue + ((newValue - oldValue) * step / ANIMATION_STEPS);

        // Clear just the number area
        display.fillRect(x, y, textSize * 6 * 8, textSize * 8, BLACK); // More space for floats
        display.setCursor(x, y);

        // Format with specific decimal places
        char buffer[10];
        dtostrf(intermediateValue, 7, decimalPlaces, buffer);
        display.print(buffer);
        display.display();
        delay(ANIMATION_DELAY);
    }
}

void updateStockData() {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;

        // Using Alpha Vantage for NASDAQ (using QQQ ETF as a proxy)
        http.begin(client, alphaVantageNasdaq);
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();

            // Parse JSON response
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, payload);

            // Extract stock value
            if (doc.containsKey("Global Quote")) {
                JsonObject quote = doc["Global Quote"];
                if (quote.containsKey("05. price")) {
                    prevNasdaq = nasdaqValue;
                    nasdaqValue = quote["05. price"].as<float>();
                }
            }
        }
        http.end();

        // Using Alpha Vantage for CAC40
        http.begin(client, alphaVantageCAC40);
        httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();

            // Parse JSON response
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, payload);

            // Extract stock value
            if (doc.containsKey("Global Quote")) {
                JsonObject quote = doc["Global Quote"];
                if (quote.containsKey("05. price")) {
                    prevCAC40 = cac40Value;
                    cac40Value = quote["05. price"].as<float>();
                }
            }
        }
        http.end();

        lastStockUpdate = millis();
    }
}

void setup() {
    Serial.begin(115200);
    delay(100); // give a little time for serial to start up

    Serial.println("Setup started");

    // Connect to Wi-Fi
    WiFiManager wifiManager;
    wifiManager.autoConnect("AirQualitySensor");

    Serial.println("Connected to WiFi");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi connected");
    display.println(WiFi.localIP());
    display.display();
    delay(2000);

    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.clearDisplay();
    display.display();
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
String getAQICategory(int aqi) {
    if (aqi <= 50) return "Good";
    else if (aqi <= 100) return "Moderate";
    else if (aqi <= 150) return "Unhealthy for Sensitive Groups";
    else if (aqi <= 200) return "Unhealthy";
    else if (aqi <= 300) return "Very Unhealthy";
    else return "Hazardous";
}

    delay(30000); // Delay for 30 seconds before the next request

    // Update stock data every 5 minutes
    if (millis() - lastStockUpdate > stockUpdateInterval || lastStockUpdate == 0) {
        updateStockData();
    }

    // Update display with animation
    display.clearDisplay();

    // Stock section - display on the right side
    display.setCursor(84, 0);
    display.println("Stocks");

    display.setCursor(84, 12);
    display.print("NSDQ");
    animateFloat(84, 24, prevNasdaq, nasdaqValue, 1, 1);

    display.setCursor(84, 36);
    display.print("CAC40");
    animateFloat(84, 48, prevCAC40, cac40Value, 1, 1);

    delay(5000); // Update every 5 seconds
}
