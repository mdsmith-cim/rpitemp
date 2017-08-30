#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Adafruit_MLX90614.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 14
#define HTTP_PORT 80
#define BUTTON_PIN 0
#define LCD_ADDR 0x3F
#define BUTTON_DEBOUNCE_TIME 500 //ms
#define TEMP_LCD_POLL 10000 //ms

// Wifi info
const char* ssid = "Sensor Net";
const char* password = "HBqSKtfrOUejsrXAM0GQkHiC";
const char* otaPassword = "updateESP8266Sensor";

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
  content += "IP Address: " + WiFi.localIP().toString() + "<br>";
  content += "SSID: " + WiFi.SSID() + "<br>";

  server.send(200, "text/html", basicHTMLResponse("Index", content));
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
  dispMLX = true;
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

  // Configure LCD for MLX display
  setupLCDMLX();

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

  // Connect to WiFi (and disable hosted AP)
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println();

  // Wait for connection
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  WiFi.printDiag(Serial);
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
  
  server.on("/", handleRoot);

  server.on("/temp", handleTempRequest);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Setting up OTA update...");

  //Setup OTA update
  ArduinoOTA.setPort(8289);

  ArduinoOTA.setPassword(otaPassword);

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

  Serial.println("Setup complete.");
}

void loop(void){

  // Handle HTTP requests
  server.handleClient();

  unsigned long curTime = millis();

  // Update temps on LCD if certain time has elapsed
  if (curTime - lastLCDTempDisplay > TEMP_LCD_POLL) {
    
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
      lcd.print(WiFi.isConnected() ? "True" : "False");
    }
  }

  // Handle update requests
  ArduinoOTA.handle();
}

