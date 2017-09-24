#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Adafruit_MLX90614.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <NTPClient.h>

// Define OTA_PASSWORD and AP_SETUP_PASSWORD, both const char*
#include <sensitive.h>

#define ONE_WIRE_BUS 14
#define HTTP_PORT 80
#define UPDATE_PORT 8289
#define BUTTON_PIN 0
#define LCD_ADDR 0x3F
#define BUTTON_DEBOUNCE_TIME 500 //ms
#define TEMP_LCD_POLL 10000 //ms
#define LCD_CHANGE 30000 //ms
#define NTP_UPDATE_INTERVAL 600000 //ms
#define WIFI_CONFIG_TIMEOUT 300 // seconds

// Taken from Michael Margolis' TimeLib - various time macros and definitions
/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define DAYS_PER_WEEK (7UL)
#define SECS_PER_WEEK (SECS_PER_DAY * DAYS_PER_WEEK)
#define SECS_PER_YEAR (SECS_PER_WEEK * 52UL)
 
/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  // this is number of days since Jan 1 1970

// HTML line break
const char* HTML_BR = "<br>";
// NTP server IP
const char* NTP_SERVER = "192.168.0.190";
// AP name for configuration
const char* AP_NAME = "Temp Sensor Setup";

// NTP client
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, NTP_SERVER);

// Create basic HTTP server
ESP8266WebServer server(HTTP_PORT);

// MLX90614 IR temp sensor
Adafruit_MLX90614 mlx = Adafruit_MLX90614(MLX90614_I2CADDR);
// 2 line 16 char display with I2C adapter
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Addresses for both temperature sensors
const DeviceAddress pumpIntake = {0x28, 0x0A, 0x62, 0xB8, 0x07, 0x00, 0x00, 0xB4};
const DeviceAddress outsideTemp = {0x28, 0xB3, 0x8E, 0x4A, 0x07, 0x00, 0x00, 0xA8};

// Variables to track button push
unsigned long last_button_press;
int volatile dispMLX;

unsigned long volatile lastLCDTempDisplay; 
unsigned long volatile lastLCDChange;

// UTC time on startup
unsigned long time_on_startup;

// Create a basic UTF-8 HTML page including header and footer
String basicHTMLResponse(String title, String content) {
  String response = "<!doctype html><html><head><title>" + title + "</title><meta charset=\"utf-8\" /><meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\" /></head><body>";
  response += (content + "</body></html>\r\n");
  return response;
}

// HTTP / request
void handleRoot() {
  String content = "Server is running.<br>";
  content += "<a href=\"temp\">Temperature page</a><br>";
  content += "<a href=\"diag\">Diagnostics page</a><br>";

  server.send(200, "text/html", basicHTMLResponse("Index", content));
}

void handleDiag() {

  unsigned long timeSinceStart = ntpClient.getEpochTime() - time_on_startup;

  String content = "<b>Dianostic Info</b><br>";
  content += "Hostname: " + WiFi.hostname() + HTML_BR;
  content += "MAC: " + WiFi.macAddress() + HTML_BR;
  content += "IP Address: " + WiFi.localIP().toString() + HTML_BR;
  content += "Subnet Mask: " + WiFi.subnetMask().toString() + HTML_BR;
  content += "Gateway: " + WiFi.gatewayIP().toString() + HTML_BR;
  content += "DNS (primary): " + WiFi.dnsIP().toString() + HTML_BR;
  content += "SSID: " + WiFi.SSID() + HTML_BR;
  content += "RSSI: " + String(WiFi.RSSI()) + HTML_BR;
  content += "Time: " + ntpClient.getFormattedTime() + " UTC" + HTML_BR;
  content += "Time since start: " + String(elapsedDays(timeSinceStart)) + " days " + String(numberOfHours(timeSinceStart)) + " hours " + String(numberOfMinutes(timeSinceStart)) + " minutes " + String(numberOfSeconds(timeSinceStart)) + " seconds" + HTML_BR;

  server.send(200, "text/html", basicHTMLResponse("Diagnostics", content));
}

void handleNotFound(){
  String message = "Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//Handle temp request
void handleTempRequest() {
  sensors.requestTemperatures(); 
  double ambTemp = mlx.readAmbientTempC();
  double objectTemp = mlx.readObjectTempC();
  float tempPump = sensors.getTempC(pumpIntake);
  float tempOutside = sensors.getTempC(outsideTemp);

  String content = "<b>Temperatures:</b><br><br>";
  content += "Ambient outdoor: ";
  content += String(ambTemp) + " °C<br>";
  content += "Lake: ";
  content += String(objectTemp) + " °C<br>";
  content += "Pump intake: ";
  content += String(tempPump) + " °C<br>";
  content += "Outside: ";
  content += String(tempOutside) + " °C<br>";

  server.send(200, "text/html", basicHTMLResponse("Temperatures", content));

}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// Initialize LCD to show MLX temp
void setupLCDMLX(void) {
  lcd.clear();
  lcd.print("H20: ");
  lcd.setCursor(0,1);
  lcd.print("Amb: ");
}

// Initialize LCD to show DS18B20 temps
void setupLCDDS18(void) {
  lcd.clear();
  lcd.print("Out: ");
  lcd.setCursor(0,1);
  lcd.print("Pmp: ");
}

// Initialize LCD to show WiFi info
void setupLCDWiFi(void) {
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Connected? ");
}

// ISR to handle button pushes (FLSH button)
void handleButton(void) {
  // Use time to avoid duplicate presses
  unsigned long curTime = millis();
  if (curTime - last_button_press > BUTTON_DEBOUNCE_TIME)
  {
    last_button_press = curTime;
    lastLCDChange = curTime;
    dispMLX = (dispMLX + 1) % 3;

    if (dispMLX == 0) {
      setupLCDMLX();
    }
    else if (dispMLX == 1) {
      setupLCDDS18();
    }
    else if (dispMLX == 2) {
      setupLCDWiFi();
    }
    // Somehow (impossibly) the variable entered a weird state; reset it
    else {
      dispMLX = 0;
    }

    // Force temp update
    lastLCDTempDisplay = 0;
  }
}

void setup(void){

  // Setup ISR for button and related variables
  dispMLX = 0;
  last_button_press = 0;
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(BUTTON_PIN, handleButton, FALLING);

  // Setup loop timer
  lastLCDTempDisplay = 0;

  // Init serial to PC
  Serial.begin(115200);

  // Init I2C to MLX sensor and sensor itself
  Wire.begin();
  mlx.begin();  

  // Init I2C to LCD
  lcd.begin();
  
  // Turn on the blacklight
  lcd.backlight();

  Serial.println();

  // Init OneWire
  sensors.begin();

  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" OneWire temp devices.");

  if (!sensors.isConnected(pumpIntake)) Serial.println("Unable to connect to pump temperature sensor!");
  if (!sensors.isConnected(outsideTemp)) Serial.println("Unable to connect to outside temperature sensor!");

  // set the resolution to 12 bits
  sensors.setResolution(12);

  // Use WiFi connection manager to hande all WiFi stuff like SSID and password
  WiFiManager wifiManager;

  Serial.println("Connecting to WiFi...");
  
  lcd.clear();
  lcd.print("AP Setup");
  
  wifiManager.setRemoveDuplicateAPs(false);
  wifiManager.setConfigPortalTimeout(WIFI_CONFIG_TIMEOUT);
  // AutoConnect will connect or launch a soft AP to allow config if it can't connect
  Serial.println(wifiManager.autoConnect(AP_NAME, AP_SETUP_PASSWORD) ? "Connected!" : "Timed out...");
  
  // Print WiFi info to serial
  WiFi.printDiag(Serial);
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
  
  // Configure LCD for MLX display
  setupLCDMLX();

  // HTTP server setup
  server.on("/", handleRoot);

  server.on("/temp", handleTempRequest);

  server.on("/diag", handleDiag);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Setting up OTA update...");

  //Setup OTA update
  ArduinoOTA.setPort(UPDATE_PORT);

  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting update...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpdate finished.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // NTP setup
  Serial.println("Setting up NTP...");

  ntpClient.begin();
  ntpClient.setUpdateInterval(NTP_UPDATE_INTERVAL);
  ntpClient.forceUpdate();

  time_on_startup = ntpClient.getEpochTime();

  Serial.println("Setup complete.");
}

void loop(void){

  // Handle HTTP requests
  server.handleClient();

  unsigned long curTime = millis();

  // Simulate button press if certain time elapsed
  if (curTime - lastLCDChange > LCD_CHANGE) {
    handleButton();
  }
  // Update temps on LCD if certain time has elapsed
  else if (curTime - lastLCDTempDisplay > TEMP_LCD_POLL) {
    
    lastLCDTempDisplay = curTime;

    sensors.requestTemperatures(); 
    double ambTemp = mlx.readAmbientTempC();
    double objectTemp = mlx.readObjectTempC();
    float tempPump = sensors.getTempC(pumpIntake);
    float tempOutside = sensors.getTempC(outsideTemp);

    Serial.print("Pump temp: ");
    Serial.print(tempPump);
    Serial.print("°C | ");
    
    Serial.print("Outside temp: ");
    Serial.print(tempOutside);
    Serial.println("°C ");
  
    Serial.print("Ambient temp: "); 
    Serial.print(ambTemp); 
    Serial.print("°C | ");
    Serial.print("Water temp: "); 
    Serial.print(objectTemp); 
    Serial.println("°C ");
  
    if (dispMLX == 0) {
      lcd.setCursor(5,0);
      lcd.print(objectTemp);
      lcd.print((char)223);
      lcd.print("C    ");
    
      lcd.setCursor(5,1);
      lcd.print(ambTemp);
      lcd.print((char)223);
      lcd.print("C    ");
    }
    else if (dispMLX == 1) {
      lcd.setCursor(5,0);
      lcd.print(tempOutside);
      lcd.print((char)223);
      lcd.print("C    ");
    
      lcd.setCursor(5,1);
      lcd.print(tempPump);
      lcd.print((char)223);
      lcd.print("C    ");
    }
    else if (dispMLX == 2) {
      lcd.setCursor(0,0);
      lcd.print(WiFi.localIP());
      
      lcd.setCursor(11,1);
      lcd.print(WiFi.isConnected() ? "Yes" : "No");
    }
  }

  // Handle update requests
  ArduinoOTA.handle();

  // Handle NTP update
  ntpClient.update();

}