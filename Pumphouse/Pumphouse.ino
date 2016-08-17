#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include "printf.h"
#include "math.h"

// Sleep functions
#include <avr/sleep.h>
#include <avr/power.h>

// Define pin used by temperature sensors
#define ONE_WIRE_BUS 7
// Use high precision for temp sensors
#define TEMPERATURE_PRECISION 12 

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

int numberOfOneWireDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

// Define known device addresses
DeviceAddress tempSensors[] = { { 0x28, 0xFF, 0xDD ,0x96,  0x36 , 0x16 , 0x03 , 0xB9 } ,{ 0x28,0xB3,0x8E,0x4A,0x07,0x00,0x00,0xA8 } ,{ 0x28,0x0A,0x62,0xB8,0x07,0x00,0x00,0xB4 } };

// Define friendly names for these addresses
String tempSensorNames[] = { "Lake Water: ", "Outside Air: ", "Inside Pumphouse: " };

// Keep track of the readings from the devices
float temperatureReadings[3];

// Singleton instance of the radio driver
// Use default pinout as they are applicable here
RH_NRF24 driver;
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini

#define myAddress 'p'

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, myAddress);

#define DATAGRAM_TIMEOUT  1000
#define RF_CHANNEL  40
#define NUM_RETRIES 3

// Payload definition
struct payload {
	uint8_t response;
	char type;
	char id;
	float value;
};

// A flag to indicate we received a message
volatile bool messageWaiting;

/********************** Setup *********************/

void setup() {

	// Set available message flag to 0 on startup
	messageWaiting = false;

	Serial.begin(115200);
	// Allow debug printing to serial
	printf_begin();

	Serial.print(F("Initializing radio..."));

	// Setup and configure rf radio
	if (!manager.init()) {
		Serial.println(F("Radio init failed"));
		return;
	}

	// Max power level
	driver.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm);

	// Random channel
	driver.setChannel(RF_CHANNEL);

	//manager.setTimeout(DATAGRAM_TIMEOUT);

	//manager.setRetries(NUM_RETRIES);

	Serial.println(F("done."));
	Serial.println(F("Intializing temperature sensors..."));

	// Initialize OneWire temp sensors
	sensors.begin();

	// Grab a count of devices on the wire
	numberOfOneWireDevices = sensors.getDeviceCount();

	Serial.print(F("Found "));
	Serial.print(numberOfOneWireDevices, DEC);
	Serial.println(F(" OneWire devices."));

	Serial.print(F("Parasite power is: "));
	if (sensors.isParasitePowerMode()) Serial.println(F("ON"));
	else Serial.println(F("OFF"));

	// Loop through each device, print out address
	for (int i = 0; i < numberOfOneWireDevices; i++)
	{
		// Search the wire for address
		if (sensors.getAddress(tempDeviceAddress, i))
		{
			Serial.print(F("Found device "));
			Serial.print(i, DEC);
			Serial.print(F(" with address: "));
			printAddress(tempDeviceAddress);
			Serial.println();

			Serial.print(F("Setting resolution to "));
			Serial.println(TEMPERATURE_PRECISION, DEC);

			// set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
			sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

			Serial.print(F("Resolution actually set to: "));
			Serial.print(sensors.getResolution(tempDeviceAddress), DEC);
			Serial.println();
		}
		else {
			Serial.print(F("Found ghost device at "));
			Serial.print(i, DEC);
			Serial.println(F(" but could not detect address. Check power and cabling"));
		}
	}
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		if (deviceAddress[i] < 16) Serial.print(F("0"));
		Serial.print(deviceAddress[i], HEX);
	}
}


// Prints the temperature of all devices and saves to RAM
void printAndSaveTemperature(uint8_t sensorId)
{
	// Make sure sensor is connected
	if (sensors.isConnected(tempSensors[sensorId])) {
		float tempC = sensors.getTempC(tempSensors[sensorId]);
		// Correct the lake temperature sensor a bit
		if (sensorId == 0) {
			tempC += 0.9;
		}
		temperatureReadings[sensorId] = tempC;
		Serial.print(tempSensorNames[sensorId]);
		Serial.print(tempC);
		Serial.println(F(" C"));
	}
	else {
		Serial.print(F("Error: unable to communicate with sensor "));
		Serial.println(tempSensorNames[sensorId]);
	}

}

// Sleep function
void enterSleep(void)
{
	Serial.println(F("Entering sleep..."));

	// Delay a bit to finish dealing with any remaining tasks like printing serial data
	delay(100);

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	sleep_enable();

	sleep_mode();

	/* The program will continue from here. */

	/* First thing to do is disable sleep. */
	sleep_disable();

	Serial.println(F("Leaving sleep..."));
}


/********************** Main Loop *********************/
void loop() {


	// Execute if there is a pending message to process
	if (manager.available()) {

		Serial.println(F("Processing received message."));

		// Wait for a message addressed to us from the client
		
		uint8_t from;
		payload recv_msg;
		uint8_t len = sizeof(recv_msg);

		if (manager.recvfromAck((uint8_t *)&recv_msg, &len, &from))
		{
			Serial.print(F("Got message from : 0x"));
			Serial.print(from, HEX);
			Serial.print(F(" | id:"));

			Serial.print(recv_msg.id);
			Serial.print(F(" | Resp:"));
			Serial.print(recv_msg.response);
			Serial.print(F(" | Type:"));
			Serial.println(recv_msg.type);

			Serial.print(F("Requesting temperatures..."));
			sensors.requestTemperatures(); // Send the command to get temperatures
			Serial.println(F("done."));

			Serial.println(F("Temperatures:"));

			// Print and save all the temperatures
			for (int i = 0; i < numberOfOneWireDevices; i++) {
				printAndSaveTemperature(i);
			}


			// Define sending payload
			payload send_msg;

			// If the message requests a temperature reading...
			if (recv_msg.response == 0 && recv_msg.type == 'T') {

				Serial.println(F("Responding..."));

				// Set type to response, of type temp
				send_msg.response = 1;
				send_msg.type = 'T';

				// Send lake, outside and inside temperature readings in succession
				send_msg.value = temperatureReadings[0];
				send_msg.id = 'L';

				bool res = manager.sendtoWait((uint8_t *)&send_msg, sizeof(send_msg), from);

				send_msg.value = temperatureReadings[1];
				send_msg.id = 'O';

				res &= manager.sendtoWait((uint8_t *)&send_msg, sizeof(send_msg), from);

				send_msg.value = temperatureReadings[2];
				send_msg.id = 'I';

				res &= manager.sendtoWait((uint8_t *)&send_msg, sizeof(send_msg), from);

				if (!res) {
					Serial.println(F("Error sending response(s)"));
				}

			}

			// Otherwise send an error message as we are not expecting other message types
			else {
				send_msg.response = 1;
				send_msg.type = recv_msg.type;
				send_msg.id = 'Z';
				send_msg.value = NAN;

				if (!manager.sendtoWait((uint8_t *)&send_msg, sizeof(send_msg), from)) {
					Serial.println(F("Error sending response(s)"));
				}
			}


		}
		else {
			Serial.println(F("Error: recvFromAck"));
		}


	}

	// Power down the CPU to save power
	delay(100);

	//enterSleep();

}


/********************** Interrupt *********************/

//void check_radio(void)
//{
//	Serial.println(F("Checking radio..."));
//
//	bool tx, fail, rx;
//	radio.whatHappened(tx, fail, rx);                     // What happened?
//
//	if (tx) {                                         // Have we successfully transmitted?
//		Serial.println(F("Payload:Sent"));
//	}
//
//	if (fail) {                                       // Have we failed to transmit?
//		Serial.println(F("Payload:Failed"));
//		if (radio.failureDetected) Serial.println("Radio failure.");
//	}
//
//	if (rx || radio.available()) {                      // Did we receive a message?
//		Serial.println(F("Message received."));
//		messageWaiting = true;
//
//	}
//}