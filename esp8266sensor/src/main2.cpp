#include <Arduino.h>
#include <ESP8266WiFi.h>
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
boolean volatile dispMLX;

unsigned long volatile lastLCDTempDisplay; 



// HTTP / request
void handleRoot() {
  server.send(200, "text/plain", "Server running...");
}

void handleNotFound(){
  String message = "File Not Found\n\n";
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

  String response = "<!doctype html>\r\n<html>\r\n<head>\r\n<title>Temperatures</title>\r\n<meta charset=\"utf-8\" />\r\n<meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\" />\r\n</head>\r\n<body>";
  response += "Temperatures:<br>";
  response += "Ambient outdoor: ";
  response += String(ambTemp) + " °C<br>";
  response += "Lake: ";
  response += String(objectTemp) + " °C<br>";
  response += "Pump intake: ";
  response += String(tempPump) + " °C<br>";
  response += "Outside: ";
  response += String(tempOutside) + " °C<br>";
  response += "</body>\r\n</html>\r\n";

  server.send(200, "text/html", response);

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

// ISR to handle button pushes (FLSH button)
void handleButton(void) {
  // Use time to avoid duplicate presses
  unsigned long curTime = millis();
  if (curTime - last_button_press > BUTTON_DEBOUNCE_TIME)
  {
    last_button_press = curTime;
    dispMLX = !dispMLX;

    if (dispMLX) {
      setupLCDMLX();
    }
    else {
      setupLCDDS18();
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

  Serial.println();
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
  
    if (dispMLX) {
      lcd.setCursor(5,0);
      lcd.print(objectTemp);
      lcd.print((char)223);
      lcd.print("C    ");
    
      lcd.setCursor(5,1);
      lcd.print(ambTemp);
      lcd.print((char)223);
      lcd.print("C    ");
    }
    else {
      lcd.setCursor(5,0);
      lcd.print(tempOutside);
      lcd.print((char)223);
      lcd.print("C    ");
    
      lcd.setCursor(5,1);
      lcd.print(tempPump);
      lcd.print((char)223);
      lcd.print("C    ");
    }
  }
}

