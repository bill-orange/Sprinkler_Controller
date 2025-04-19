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
03/12/2025 Added more conditions that will display error message
03/14/2025 Minors
03/17/2025 Moved explaination prompt into github Started OTA work
03/18/2025 OTA and posting of last message added. Compile with Minimal SPIFFS
03/22/2025 Turn off green LED and blink it on after AI explain,
           Minors
03/26/2025 Forked for addding soil resistance -- Mostly UNTESTED
04/03/2025 Impoved Web Page - Testing
04/04/2025 Change Scaling to float - Still Testing
04/05/2025 Simplified and corrected server.on & lengthened timeout in support_function.h
04/07/2025 Added confused AI png file, Added tempString to web report - UNTESTED
04/10/2025 fixed variable names and scope errors
04/12/2024 Made String storage area a bit larger and compiled with new JSON library
04/14/2025 Greatly increased timouts on HTTP incuding within weathercall library.
04/17/2025 Tokens increased and minors

*/
//#define USE_LINE_BUFFER     // Enable for faster rendering
#define debug true          // eliminates print statements
#include <WiFi.h>           // Needed for Version 3 Board Manager
#include <WiFiMulti.h>      // Needed for more than one possible SSDI
#include <TFT_eSPI.h>       // Hardware-specific library
#include "FS.h"             // For SD Card
#include "SD.h"             // For SD Card
TFT_eSPI tft = TFT_eSPI();  // For Display

#include <HTTPClient.h>         // Get Web Data
#include <WiFiClient.h>         // For OTA
#include <WebServer.h>          // For OTA
#include <ElegantOTA.h>         // OTA Library ESP32 compatable
#include "support_functions.h"  // Process PNG for TFT display
#include <weathercall.h>        // Get Open WeatherMap data
#include <ChatGPTuino.h>        // For AI Support
#include <CRC8.h>               // CRC8 library
#include <CRC.h>                // CRC library
#include "secrets.h"            //  Just what the name implies plus location info

SPIClass hspi(HSPI);  // Start SPI instance

SET_LOOP_TASK_STACK_SIZE(14 * 1024);  // needed to handle really long strings

#define DISPLAYBAUD 115200                         // set for LoRa communication/#define DISPLAYBAUD 115200
#define RELAY_PIN 5                                // Define the pin connected to the relay
#define LED_PIN 21                                 // only likely correct for T8 board
#define MYPORT_TX 22                               // Transmit pin for serial
#define MYPORT_RX 27                               // Receive pin for serial
#define ADDRESS 2                                  // Address of receiving LoRa.  Must be in same Network
#define TIME_Between_Weather_Calls (3600000 * 12)  // Every 12 Hours
//#define TIME_Between_Weather_Calls (3600000 * 1)  // Every 1 Hour -- For Testing Only!
//#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

String latitude = LAT;               // 90.0000 to -90.0000 negative for Southern hemisphere
String longitude = LON;              // 180.000 to -180.000 negative for West
String units = Units;                // or "imperial"
String language = Language;          // See notes tab
String jsonBuffer;                   // Storage for JSON String test, if needed
String tempString2 = " pending   ";  // The explaination of Yes/No reply
String tempString = " pending   ";   // The binary Yes/No reply
String incomingstring = "void";      // Unparsed LoRa message
String message = "unknown";          // Web message
String partialRealUserMessage;
String soilMessage = "200";             // Data from soil moisture transmitter
String signal_st = "void";              // Signal strenght in dB for some reason the word signal
                                        // is reserved in the ESP32 compiler
String sn = "00";                       // Signal-to-Noise Ratio
String lora_trans_prefix = "AT+SEND=";  // LoRa prefix send without CR/LF

char *AIShortReply = "                                                          ";  // AI Reply storage


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

const int TOKENS = 220;             // How lengthy a response you want, every token is about 3/4 a word
const int NUM_MESSAGES = 30;        // Another budget limit
const char *model = "gpt-4o-mini";  // OpenAI Model being used
//const char *model = "gpt-4o";     // OpenAI Model being used
const char *filename = "/AI_data.txt";

unsigned long previousMillis = 0;                           // Stores the last time the task was executed
unsigned long ota_progress_millis = 0;                      // OTA timer
const unsigned long interval = TIME_Between_Weather_Calls;  // 12-hour interval in milliseconds

int firstTime = 1;  // Run main program on startup or error

weatherData w;                                     // Instance of OpenWeatherMap retriever
Weathercall weather(apiKeyOpenWeather, location);  // Setup fetch
Weathercall forecast(apiKeyOpenWeather, location, 1);

ChatGPTuino chat{ TOKENS, NUM_MESSAGES };  // Will store and send your most recent messages (up to NUM_MESSAGES)
WiFiMulti wifiMulti;                       // Constructor for multiple wifi ssid
WebServer server(80);                      // OTA web server instance
CRC8 crc;                                  // Instance of CRC calculator


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
  Serial2.begin(DISPLAYBAUD, SERIAL_8N1, MYPORT_RX, MYPORT_TX);
  delay(3000);
  Serial.println("Setup Running.......");
  Serial.print("compiler Version: ");
  Serial.println(__cplusplus);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);  // Blink LED on data received
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
  digitalWrite(TFT_BL, LOW);  // Green LED off
  showGraphic("wait_for_it.png", 1);
  configureServer();  // Setup the server route
  server.begin();     // Start the server

  ElegantOTA.begin(&server);  // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");

  String incomingstring_check = " ";  // Keep the incoming checksone string local

  sleepMode(0);  // Want the transmitting LoRA to be awake
  delay(2000);

  Serial2.println("AT+RESET");
  delay(2000);
  Serial2.println("AT");
  do {


    if (Serial2.available()) {
      incomingstring_check = Serial2.readString();
    }

  } while (incomingstring_check.indexOf("+OK") == -1);  // Confusing.. This is a double negative

  Serial.print("Test before startup: ");
  Serial.println(incomingstring_check);

  Serial.println("------------------------------------------------------------------------");
  Serial.println(" Setup Complete ................ Getting Data");
}

/*------------------------------------------- Loop -----------------------------------------------*/
void loop() {

  server.handleClient();
  ElegantOTA.loop();
  getSoilMoisture();

  if (firstTime == 1) {

    if (main() == 1) {  // Run main after TIME_Between_Weather_Calls and recycle on error
      showGraphic("error.png", 1);
      firstTime = 1;
      yield();
    } else {
      firstTime = 0;
    }
  }

  // Check if 12 hours have passed
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= TIME_Between_Weather_Calls) {
    previousMillis = currentMillis;

    WiFi.disconnect();  // Re-establish network connection, just to be shure
    delay(2000);
    connectToWifiNetwork();
    delay(2000);

    //ESP.restart();      // Restart the ESP32

    if (main() == 1) {  // Run main after TIME_Between_Weather_Calls and recycle on error
      showGraphic("error.png", 1);
      firstTime = 1;
      yield();
    }
  }
  yield();  // Get ready for OTA call
}

/* ----------------------------------- Main loop separated from Lopp for non-blocking -------------------------*/
int main() {
  int consultAINeeded = 1;    // yes
  int error = 0;              // Tracks errors between functions
  weather.updateStatus(&w);   // Fetches weather
  forecast.updateStatus(&w);  // Fetches forecast

  if (strstr(w.weather.c_str(), "ain") != NULL || strstr(w.weather1.c_str(), "ain") != NULL || strstr(w.weather2.c_str(), "ain") != NULL) {
    consultAINeeded = 0;
    Serial.println("RAIN ............ RAIN ............... RAIN  >>>> No forecast needed");
    delay(3000);  // don't want to make server calls too close together
    showGraphic("12.png", 0);
  }
  if (consultAINeeded == 1) {
    yield();
    showGraphic("AI_computer_eye.png", 1);
    tempString = AIForecast();
    Serial.println("*****************************");
    Serial.println(tempString);
    Serial.println("*****************************");

    if (tempString.indexOf("NO") != -1) {
      showGraphic("11.png", 1);
    } else if (tempString.indexOf("YES") != -1) {
      showGraphic("12.png", 0);
    } else {
      showGraphic("confused_AI.png", 1);
    }

    if (debug && consultAINeeded == 1) {
      tempString2 = AIExplain();
      //configureServer();

      Serial.println(tempString2);
      const char *cstr = tempString2.c_str();
      appendFile(SD, filename, cstr);
      appendFile(SD, filename, "\n\n");
    }
    if (debug && consultAINeeded == 0) {
      appendFile(SD, filename, "rain ... rain ... rain \n\n");
    }
  }
  if (w.current_Temp == 0 || w.current_Temp1 == 0) {
    error = 1;
  }
  configureServer();
  yield();
  return error;
}

/*-------------------------------- Connect to the Wifi network ------------------------------------*/
void connectToWifiNetwork() {  // Boilerplate from example (mostly)

  delay(10);
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid_1, password_1);
  wifiMulti.addAP(ssid_2, password_2);

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    showGraphic("error.png", 1);
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
    WiFi.setAutoReconnect(true);  // Reconnect if disconnected
    WiFi.persistent(true);        // Reconnect if disconnected
    showGraphic("WiFiConnected.png", 1);
  } else {  // Handle error
    showGraphic("error.png", 1);
    delay(6000);
  }
}

/*-------------------------------- Gets Inhibit Criteria from Github -------------------------------------*/
String criteria() {  // Mostly from example
  uint32_t t = millis();
  HTTPClient http;
  http.setTimeout(120000);
  http.begin("https://raw.githubusercontent.com/bill-orange/Sprinkler_Controller/master/criteria.txt");

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP ERROR: %d\n", httpCode);
    showGraphic("error.png", 1);
    http.end();
    return "   ";
  }

  String payload = http.getString();  // Read response into a String
  http.end();                         // Close connection

  t = millis() - t;
  Serial.print(t);
  Serial.println(" ms to load URL");
  yield();
  return payload;
}

/*-------------------------------- Gets Inhibit Explaination from Github -------------------------------------*/
String explain() {  // Written by AI
  uint32_t t = millis();
  HTTPClient http;
  http.begin("https://raw.githubusercontent.com/bill-orange/Sprinkler_Controller/master/explain.txt");

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP ERROR: %d\n", httpCode);
    showGraphic("error.png", 1);
    http.end();
    return "   ";
  }

  String payload = http.getString();  // Read response into a String
  http.end();                         // Close connection

  t = millis() - t;
  Serial.print(t);
  Serial.println(" ms to load URL");
  yield();
  return payload;
}

/*----------------------------------- Send Prompt to AI ----------------------------------------*/
String AIForecast() {
  // Create the structures that hold the retrieved weather
  String realUserMessage = String(criteria())
                           + " Soil resistivity is: " + soilMessage + " Ohm-Meter\n"
                           + "Future forecast is provided in this JSON String\n"
                           + String(forecast.getResponse().c_str())
                           + " Current  weather is provided in this JSON String\n"
                           + String(weather.getResponse().c_str());  // User message to ChatGPT

  partialRealUserMessage = String(criteria())
                           + " Current weather is provided in this JSON String (redacted)\n"
                           + " Future forecast weather is provided in this JSON String (redacted)\n"
                           + " Soil resistance given to AI is: " + soilMessage + " Ohm-Meter.";


  // partialRealUserMessage = realUserMessage;

  Serial.println(realUserMessage);  // This and the lines below taken from library example
  int str_len = realUserMessage.length() + 1;
  char AIPrompt[str_len];
  realUserMessage.toCharArray(AIPrompt, str_len);
  GetAIReply(AIPrompt);
  //Serial.println(GetAIReply(AIPrompt));
  yield();
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
  String realUserMessage = explain();

  Serial.println(realUserMessage);  // This and the lines below taken from library example
  int str_len = realUserMessage.length() + 1;
  char AIPrompt[str_len];
  realUserMessage.toCharArray(AIPrompt, str_len);
  GetAIReply(AIPrompt);
  digitalWrite(TFT_BL, HIGH);  // Green LED on, Bink to indicate explaination is ready
  delay(2000);
  digitalWrite(TFT_BL, LOW);  // Green LED off
  return String(GetAIReply(AIPrompt));
}

/*---------------------------- File information  ------------------------------------------*/
void fileInfo() {  // uesful to figure our what software is running


  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE);  // Print to TFT display, White color
  tft.setTextSize(1);
  tft.drawString("    Sprinkler Inhibitor with OTA", 8, 60);
  tft.drawString("    AI Weather Prediction", 30, 70);
  tft.setTextSize(1);
  tft.drawString(__FILENAME__, 35, 1100);
  tft.drawString(__DATE__, 35, 140);
  tft.drawString(__TIME__, 125, 140);
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

/*---------------------------- Helper Functions for OTA----------------------------------------*/
void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

/*----------------------------Send message to web page----------------------------------------*/
void configureServer() {
  server.on("/", [&tempString2, &soilMessage, &tempString, &partialRealUserMessage]() {  // Capture by reference
    String message = "Hi! This is Sprinkler Controller. use /update for UPDATE \n\n Current Soil Resistance: "
                     + soilMessage
                     + " \n\n Latest Yes/No Decision: "
                     + tempString
                     + " \n\n Latest prompt: \n\n"
                     + partialRealUserMessage
                     + "\n\n Latest reply: \n\n"
                     + tempString2;

    server.send(200, "text/plain", message);
  });
  yield();
}

/*------------------------------- Parse Message ------------------------------------*/
void parseMessage() {

  // Very standard C++ parser for comma delimited String

  int first = incomingstring.indexOf(",");
  int second = incomingstring.indexOf(",", first + 1);
  int third = incomingstring.indexOf(",", second + 1);
  int fourth = incomingstring.indexOf(",", third + 1);
  int fifth = incomingstring.indexOf("\r", fourth + 1);

  soilMessage = incomingstring.substring(second + 1, third);
  signal_st = incomingstring.substring(third + 1, fourth);
  sn = incomingstring.substring(fourth + 1, fifth);
}

/*---------------------------------------------- CRC CAlculator ------------------------------------------------*/

uint8_t CRCCalculator(String message) {

  // CRC8 calculator see libraries examples on Github

  crc.reset();
  char char_array[message.length() + 1];
  message.toCharArray(char_array, message.length());
  crc.add((uint8_t *)char_array, message.length());
  return crc.calc();
}

/*---------------------------------------------- Data Transmitter ------------------------------------------------*/

void DataTransmitter(String message) {

  uint8_t crc = CRCCalculator(message);

  String CRCMessage = String(crc);

  Serial2.print(lora_trans_prefix);
  Serial2.print(ADDRESS);
  Serial2.print(",");
  Serial2.print(CRCMessage.length());
  Serial2.print(",");
  Serial2.println(CRCMessage);

  String incomingstring_check = " ";  // Keep the incoming checksone string local

  do {
    if (Serial2.available()) {
      incomingstring_check = Serial2.readString();
    }

  } while (incomingstring_check.indexOf("+OK") == -1);  // Confusing.. This is a double negative

  Serial.println("  ");
  Serial.print("Test before Transmit: ");
  Serial.println(incomingstring_check);

  yield();  // Don't want a timeout
}


/*-------------------------------------- RYL988 Sleep Mode ---------------------------------------------*/

void sleepMode(int snooze) {

  if (snooze == 0) {
    DataTransmitter("AT+MODE=0");
    Serial.println("                      REL988 out of sleep");
    delay(2000);
  } else {
    DataTransmitter("AT+MODE=1");
    Serial.println("                      REL988 in sleep");
    delay(2000);
  }
}

/*----------------------------------- Get the Soil Moisture -----------------------------------------*/

void getSoilMoisture() {

  digitalWrite(LED_PIN, LOW);  // turn the LED OFF waiting for data

  //sleepMode(0);
  //delay(2000);
  if (Serial2.available()) {
    incomingstring = Serial2.readString();

    if (incomingstring.indexOf("+OK") == -1) {  // Strip out all the +OK
      digitalWrite(LED_PIN, HIGH);              // turn the LED  ON processing data
      parseMessage();                           // break unformation in incoming string in to useful Strings
      DataTransmitter(soilMessage);             // Now that we have the message we can transmitt the CRC

      float soilMessagef = soilMessage.toFloat();
      soilMessagef = soilMessagef / 3.5;
      //integerSoilMessage = constrain(integerSoilMessage, 500, 2000);
      //integerSoilMessage = map(integerMessage, 500, 2000, 175, 1000);
      soilMessage = String(soilMessagef);
      Serial.print("                                    Soil Resistance 0-1000 ohms-meter: ");
      Serial.println(soilMessage);
      Serial.print("                                    Signal_strength: ");
      Serial.print(signal_st);
      Serial.print("dB");

    } else {
      Serial.print("rejected message: ");
      Serial.println(incomingstring);
    }
  }
  //sleep(1);
  //Serial.print(".");
  yield();  // Housekeeping
}
/*-----------------------------------------------------------------------------------*/
