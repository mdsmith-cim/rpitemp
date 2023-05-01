#include <Arduino.h>
/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 * 
 * Modifications for personal use
 * Michael Smith, Dec. 2020
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 *
 * Based off of example sketch showing how to send in DS1820B OneWire temperature readings back to the controller
 * http://www.mysensors.org/build/temp
 */


// Enable debug prints to serial monitor
#define MY_DEBUG 

#define MY_BAUD_RATE 9600

// Enable and select radio type attached
#define MY_RADIO_RF24
#define	MY_RF24_CE_PIN 9
#define MY_RF24_CS_PIN 10
#define MY_RF24_PA_LEVEL RF24_PA_MAX
#define MY_RF24_CHANNEL 125

#define LED_GREEN 2
#define LED_BLUE 3
#define LED_RED 4

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 5000

// Flash leds on rx/tx/err
// Led pins used if blinking feature is enabled above
#define MY_DEFAULT_ERR_LED_PIN LED_RED  // Error led pin
#define MY_DEFAULT_RX_LED_PIN LED_BLUE  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN LED_GREEN  // the PCB, on board LED

#include <MySensors.h>  
#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS 7 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16

unsigned long SLEEP_TIME = 300000; // Sleep time between reads (in milliseconds)

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 
bool receivedConfig = false;
bool metric = true;
uint8_t numSensors = 0;
// Initialize temperature message
MyMessage msg(1, V_TEMP);
MyMessage msgVolt(0, V_VOLTAGE);

// Battery level
#include <Battery.h>
Battery batt = Battery(2500, 4200, A0);

void before()
{
  // Startup up the OneWire library
  sensors.begin();
  batt.begin(5100, 1.0, &sigmoidal);
}

void setup()  
{ 
  analogReference(DEFAULT);
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Backyard Temp Sensor", "1.0");

  //Present battery voltage level
  present(0, S_MULTIMETER);

  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();
  CORE_DEBUG(PSTR("NUM TEMP=%" PRIu8 "\n"), numSensors);

  // Present all sensors to controller
  for (int i=0; i< numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(i+1, S_TEMP);
  }

}

void loop()     
{     
  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  sleep(conversionTime);

  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
 
    // Fetch temp
    float temperature = getControllerConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i);
 
    // Send in the new temperature
    // Error if -127.0 or 85, but let that pass through to controller for debug purposes
    send(msg.setSensor(i+1).set(temperature, 2));
    
  }

  // get the battery status
  uint16_t battVoltage = batt.voltage();
  uint8_t battLevel = batt.level(battVoltage);
  CORE_DEBUG(PSTR("BATT:mV=%" PRIu16 ",LVL=%" PRIu8 "\n"), battVoltage, battLevel);
  sendBatteryLevel(battLevel);
  send(msgVolt.set(battVoltage / 1000.0f, 3));

  // Wait for next sensor probe
  sleep(SLEEP_TIME);
}