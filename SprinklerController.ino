/* William E Webb (c) MIT LICENSE Sprinker Controller Inhibit using OpenWeatherMap and ChatGPT
   
   This project uses TFT_eSPI by Bodmer The original starting point for this library was the
   Adafruit_ILI9341 library in January 2015. MIT License
   
   This project uses weathercall by François Crins 2nd. MIT License/
   
   This project uses ChatGPTuino by programming-electronics-acadamy 
   GNU Lesser General Public License v2.1

02/12/2025 First release - runs and appears to work but without rain, 
           I can not test the flow control.  Support_functions.h has been modified.
02/17/2025 Added fileInfo to startup.  Added WiFi Multi.
02/18/2025 Added MIT License
02/19/2025 Added Graphic to show WiFi is connected.
02/22/2025 Added explaination of decision print to serial
           Can not print to SD because of pin duplication
02/25/2025 Improved download speed in support_functions.h revised
02/26/2025 Clarify rain/sun if statement
03/02/2025 Move DC pin from GPIO2 to GPIO19 to free up SD card I/O
           Added SD card support of storing AI messages. Untested!
03/03/2025 Added uSD Card loading graphics
           Added Generalized Graphics function
03/02/2025 Minors
03/02/2025 Minors
03/07/2025 Minors

*/
#define USE_LINE_BUFFER  // Enable for faster rendering
#define debug true       // eliminates print statements
#include <WiFi.h>        // Needed for Version 3 Board Manager
#include <WiFiMulti.h>
#include <TFT_eSPI.h>  // Hardware-specific library
//#include <SPI.h>            // needs testing may not be needed
#include "FS.h"             // For SD Card
#include "SD.h"             // For SD Card
TFT_eSPI tft = TFT_eSPI();  // For Display

#include <HTTPClient.h>         // Get Web Data
#include "support_functions.h"  // Process PNG for TFT display
//#include <ArduinoJson.h>        // Needed to decode weather JSON
#include <weathercall.h>  // Get Open WeatherMap data
#include <ChatGPTuino.h>  // For AI Support
#include "secrets.h"      //  Just what the name implies plus location info

SPIClass hspi(HSPI);

SET_LOOP_TASK_STACK_SIZE(12 * 1024);  // needed to handle really long strings

#define RELAY_PIN 5                                // Define the pin connected to the relay
#define TIME_Between_Weather_Calls (3600000 * 12)  // Every 12 Hours
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
String latitude = LAT;       // 90.0000 to -90.0000 negative for Southern hemisphere
String longitude = LON;      // 180.000 to -180.000 negative for West
String units = Units;        // or "imperial"
String language = Language;  // See notes tab
String jsonBuffer;           // Storage for JSON String test if needed
char *AIShortReply = "                                                             ";

// REASSIGN_PINS for uSD
int sck = 14;
int miso = 2;
int mosi = 15;
int cs = 13;

/*  For reference only do not uncomment - pin assigments for TFT Display
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   12  // Chip select control pin
#define TFT_DC   19  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
*/

const int TOKENS = 174;             // How lengthy a response you want, every token is about 3/4 a word
const int NUM_MESSAGES = 30;        // Another budget limit
const char *model = "gpt-4o-mini";  // OpenAI Model being used
//const char *model = "gpt-4o";  // OpenAI Model being used
const char *filename = "/AI_data.txt";

weatherData w;                                     // Instance of OpenWeatherMap retriever
Weathercall weather(apiKeyOpenWeather, location);  // Setup fetch
Weathercall forecast(apiKeyOpenWeather, location, 1);

ChatGPTuino chat{ TOKENS, NUM_MESSAGES };  // Will store and send your most recent messages (up to NUM_MESSAGES)
WiFiMulti wifiMulti;                       // Constructor for multiple wifi ssid

/*-------------------------------- Display Graphics ----------------------------------------*/
void showGraphic(String(image), int(relayState)) {
  uint32_t t = millis();

  setPngPosition(0, 0);
  String githubURL = GITHUBURL + image;
  const char *URLStr = githubURL.c_str();
  Serial.println(URLStr);
  load_png(URLStr);
  t = millis() - t;
  Serial.print(t);
  Serial.println(" ms to load URL");
  updateRelay(relayState);
}

/*-------------------------------- Setup ------------------------------------*/
void setup() {

  Serial.begin(115200);
  delay(20);
  Serial.print("compiler Version: ");
  Serial.println(__cplusplus);
  pinMode(RELAY_PIN, OUTPUT);
  updateRelay(1);
  tft.begin();
  tft.fillScreen(0);
  tft.setRotation(2);
  fileInfo();
  connectToWifiNetwork();
  delay(3000);
  chat.init(chatGPT_APIKey, model);  // Initialize AI chat

  hspi.begin(sck, miso, mosi, cs);
  delay(2000);

  // Initialize SD card
  if (!SD.begin(cs, hspi)) {
    Serial.println("SD card initialization failed!");
    showGraphic("SD_No.png", 1);
  } else {
    Serial.println("SD card initialized.");
    showGraphic("SD_Yes.png", 1);
  }
  delay(3000);
  // Check if the file exists
  if (SD.exists(filename)) {
    Serial.println("File exists.");
  } else {
    Serial.println("File does not exist. Creating file...");
    // Create the file
    File file = SD.open(filename, FILE_WRITE);
    if (file) {
      Serial.println("File created.");
      file.close();
    } else {
      Serial.println("Failed to create file.");
    }
  }
  //showGraphic("WIP.png", 1);
  showGraphic("wait_for_it.png", 1);
}

/*------------------------------------------- Loop -----------------------------------------------*/
void loop() {
  int consultAINeeded = 1;  // yes

  weather.updateStatus(&w);   // Fetches weather
  forecast.updateStatus(&w);  // Fetches forecast

  if (strstr(w.weather.c_str(), "ain") != NULL || strstr(w.weather1.c_str(), "ain") != NULL || strstr(w.weather2.c_str(), "ain") != NULL) {
    consultAINeeded = 0;
    Serial.println("RAIN ............ RAIN ............... RAIN  >>>> No forecast needed");
    showGraphic("12.png", 0);
  }
  if (consultAINeeded == 1) {
    showGraphic("AI_computer_eye.png", 1);
    String tempString = AIForecast();
    Serial.println("*****************************");
    Serial.println(tempString);
    Serial.println("*****************************");

    if (tempString.indexOf("NO") != -1) {
      showGraphic("11.png", 1);
    } else {
      showGraphic("12.png", 0);
    }

    if (debug && consultAINeeded == 1) {
      String tempString2 = AIExplain();
      Serial.println(tempString2);
      const char *cstr = tempString2.c_str();
      appendFile(SD, filename, cstr);
      appendFile(SD, filename, "\n\n");
    }
    if (debug && consultAINeeded == 0) {
      appendFile(SD, filename, "rain ... rain ... rain \n\n");
    }
  }

  yield();

  if (w.current_Temp != 0 && w.current_Temp1 != 0) {
    delay(TIME_Between_Weather_Calls);
  }  // Recycle on error
  else {
    showGraphic("error.png", 1);
    yield();
  }
}

/*-------------------------------- Connect to the Wifi network ------------------------------------*/
void connectToWifiNetwork() {

  delay(10);
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid_1, password_1);
  wifiMulti.addAP(ssid_2, password_2);

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }

  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    showGraphic("WiFiConnected.png", 1);
  }
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

/*----------------------------------- Send Explaination Prompt to AI ----------------------------------------*/
String AIExplain() {
  // Create the structures that hold the retrieved weather
  String realUserMessage = "Please explain how you arrived at your Yes/No answer. Keep the explaination under 40 words.  Preface your reply with the time and date taken form the current conditions JSON String. For this reply you are not bound by the Yes/No restriction";

  Serial.println(realUserMessage);  // This and the lines below taken from library example
  int str_len = realUserMessage.length() + 1;
  char AIPrompt[str_len];
  realUserMessage.toCharArray(AIPrompt, str_len);
  GetAIReply(AIPrompt);
  //Serial.println(GetAIReply(AIPrompt));
  return String(GetAIReply(AIPrompt));
}

/*---------------------------- File information  ------------------------------------------*/
void fileInfo() {  // uesful to figure our what software is running


  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE);  // Print to TFT display, White color
  tft.setTextSize(1);
  tft.drawString("    AI openWeather Test ", 25, 50);
  tft.drawString("    Demos AI Prediction", 25, 70);
  tft.setTextSize(1);
  tft.drawString(__FILENAME__, 30, 1100);
  tft.drawString(__DATE__, 30, 140);
  tft.drawString(__TIME__, 120, 140);
  delay(3000);
}

/*---------------------------- Append File  ----------------------------------------*/
void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

/*--------------------------------------------------------------------------------------------------*/
