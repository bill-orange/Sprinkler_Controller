/* William E Webb (c) Sprinker Controller Inhibit using OpenWeatherMap and ChatGPT
   
   This project uses TFT_eSPI by Bodmer The original starting point for this library was the
   Adafruit_ILI9341 library in January 2015. MIT License
   
   This project uses weathercall by François Crins 2nd. MIT License/
   
   This project uses ChatGPTuino by programming-electronics-acadamy 
   GNU Lesser General Public License v2.1

02/12/2024 First release - runs and appears to work but without rain, 
           I can not test the flow control.  Support_functions.h has been modified.

*/
#define USE_LINE_BUFFER                            // Enable for faster rendering

#include <WiFi.h>           // Needed for Version 3 Board Manager
#include <TFT_eSPI.h>       // Hardware-specific library
//#include <SPI.h>            // needs testing may not be needed
TFT_eSPI tft = TFT_eSPI();  // For Display

#include <HTTPClient.h>         // Get Web Data
#include "support_functions.h"  // Process PNG for TFT display
//#include <ArduinoJson.h>        // Needed to decode weather JSON
#include <weathercall.h>        // Get Open WeatherMap data
#include <ChatGPTuino.h>        // For AI Support
#include "secrets.h"            //  Just what the name implies plus location info

SET_LOOP_TASK_STACK_SIZE(12 * 1024);  // needed to handle really long strings

#define RELAY_PIN 5                                // Define the pin connected to the relay
#define TIME_Between_Weather_Calls (3600000 * 12)  // Every 12 Hours
String latitude = LAT;                             // 90.0000 to -90.0000 negative for Southern hemisphere
String longitude = LON;                            // 180.000 to -180.000 negative for West
String units = Units;                              // or "imperial"
String language = Language;                        // See notes tab
String jsonBuffer;                                 // Storage for JSON String test if needed
char *AIShortReply = "                                                             ";

const int TOKENS = 174;             // How lengthy a response you want, every token is about 3/4 a word
const int NUM_MESSAGES = 30;        // Another budget limit
const char *model = "gpt-4o-mini";  // OpenAI Model being used

weatherData w;                                     // Instance of OpenWeatherMap retriever
Weathercall weather(apiKeyOpenWeather, location);  // Setup fetch
Weathercall forecast(apiKeyOpenWeather, location, 1);

ChatGPTuino chat{ TOKENS, NUM_MESSAGES };  // Will store and send your most recent messages (up to NUM_MESSAGES)

/*-------------------------------- Setup ------------------------------------*/
void setup() {

  Serial.begin(115200);
  delay(20);
  Serial.print("compiler Version: ");
  Serial.println(__cplusplus);
  connectToWifiNetwork();
  tft.begin();
  tft.fillScreen(0);
  tft.setRotation(2);
  WIP();
  pinMode(RELAY_PIN, OUTPUT);
  updateRelay(1);
  chat.init(chatGPT_APIKey, model);  // Initialize AI chat
}

/*-------------------------------- Loop ------------------------------------*/
void loop() {
  int consultAINeeded = 1;  // yes

  weather.updateStatus(&w);   // Fetches weather
  forecast.updateStatus(&w);  // Fetches forecast

  if ( strstr(w.weather.c_str(), "ain") != NULL || strstr(w.weather1.c_str(), "ain") != NULL || strstr(w.weather2.c_str(), "ain") != NULL) {
    consultAINeeded = 0;
    Serial.println("RAIN ............ RAIN ............... RAIN  >>>> No forecast needed");
    rain();
  }
  if (consultAINeeded == 1) {
    String tempString = AIForecast();
    Serial.println("*****************************");
    Serial.println(tempString);
    Serial.println("*****************************");

    if (strstr(tempString.c_str(), "NO") != NULL) {
      sun();
    } else {
      rain();
    }
  }

  yield();

  if (w.current_Temp != 0 && w.current_Temp1 != 0) {
    delay(TIME_Between_Weather_Calls);
  }  // Recycle on error
  else {
    error();
    yield();
  }
}

/*-------------------------------- Connect to the Wifi network ------------------------------------*/
void connectToWifiNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/*-------------------------------- Display Rain Graphic ------------------------------------*/
void rain() {
  uint32_t t = millis();

  setPngPosition(0, 0);
  load_png("https://raw.githubusercontent.com/bill-orange/Sprinkler_Controller/master/data/12.png");
  t = millis() - t;
  Serial.print(t);
  Serial.println(" ms to load URL");
  updateRelay(0);
}

/*-------------------------------- Display Sun Graphic -------------------------------------*/
void sun() {
  uint32_t t = millis();

  setPngPosition(0, 0);
  load_png("https://raw.githubusercontent.com/bill-orange/Sprinkler_Controller/master/data/11.png");
  t = millis() - t;
  Serial.print(t);
  Serial.println(" ms to load URL");
  updateRelay(1);
}

/*-------------------------------- Display Sun Graphic -------------------------------------*/
void error() {
  uint32_t t = millis();

  setPngPosition(0, 0);
  load_png("https://raw.githubusercontent.com/bill-orange/Sprinkler_Controller/master/data/error.png");
  t = millis() - t;
  Serial.print(t);
  Serial.println(" ms to load URL");
}

/*-------------------------------- Display Sun Graphic -------------------------------------*/
void WIP() {
  uint32_t t = millis();

  setPngPosition(0, 0);
  load_png("https://raw.githubusercontent.com/bill-orange/Sprinkler_Controller/master/data/WIP.png");
  t = millis() - t;
  Serial.print(t);
  Serial.println(" ms to load URL");
}
/*-------------------------------- Gets Inhibit Criteria from Github -------------------------------------*/
String criteria() {
  uint32_t t = millis();
  HTTPClient http;
  http.begin("https://raw.githubusercontent.com/bill-orange/Sprinkler_Controller/master/criteria.txt");

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP ERROR: %d\n", httpCode);
    http.end();
    return "   ";
  }

  String payload = http.getString();  // Read response into a String
  http.end();                         // Close connection

  t = millis() - t;
  Serial.print(t);
  Serial.println(" ms to load URL");
  return payload;
}

/*----------------------------------- Send Prompt to AI ----------------------------------------*/
String AIForecast() {
  // Create the structures that hold the retrieved weather
  String realUserMessage = String(criteria())
                           + String(forecast.getResponse().c_str())
                           + " Future forecast weather is provided in this JSON String"
                           + String(weather.getResponse().c_str());  // User message to ChatGPT


  Serial.println(realUserMessage);  // This and the lines below taken from library example
  int str_len = realUserMessage.length() + 1;
  char AIPrompt[str_len];
  realUserMessage.toCharArray(AIPrompt, str_len);
  GetAIReply(AIPrompt);
  //Serial.println(GetAIReply(AIPrompt));
  return String(GetAIReply(AIPrompt));
}

/*----------------------------------- Process AI Call ----------------------------------------*/
char *GetAIReply(char *message) {  // Function taken from libary example
  chat.putMessage(message, strlen(message));
  chat.getResponse();
  return chat.getLastMessageContent();
}

/*----------------------------------- Relay Control ------------------------------------------*/
void updateRelay(int yesNo) {
  if (yesNo == 1) {
    digitalWrite(RELAY_PIN, HIGH);  // Close relay
  } else {
    digitalWrite(RELAY_PIN, LOW);  // Open relay
  }
}
