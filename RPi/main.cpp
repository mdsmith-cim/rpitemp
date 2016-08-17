/*
        TMRh20 2014 - Optimized RF24 Library Fork
 */

/**
 * Example using Dynamic Payloads
 *
 * This is an example of how to use payloads of a varying (dynamic) size.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include "RHReliableDatagram.h"
#include "RH_NRF24.h"
#include "wiringPi.h"
#include <signal.h>

using namespace std;

#define DATAGRAM_TIMEOUT  1000
#define RF_CHANNEL  40
#define NUM_RETRIES 3

#define MY_ADDRESS 'c'
#define PUMP_ADDRESS 'p'

#define CE_PIN 16
#define CS_PIN 18

RH_NRF24 driver(CE_PIN, CS_PIN);
RHReliableDatagram manager(driver, MY_ADDRESS);

uint8_t num_packets_expected;
// Payload definition

struct __attribute__((packed)) payload {
    uint8_t response;
    char type;
    char id;
    float value;
};

void sig_handler(int sig);
void getAndPrintResponse();

volatile sig_atomic_t flag = 0;

int main(int argc, char** argv) {

    signal(SIGINT, sig_handler);

    string command_base = "gpio export ";
    string cmd1 = command_base.append(to_string(CE_PIN)).append(" out");
    int system_result = system(cmd1.c_str());

    string cmd2 = command_base.replace(command_base.find(to_string(CE_PIN)), 2, to_string(CS_PIN));
    system_result += system(cmd2.c_str());


    if (system_result != 0) {
        printf("Error with gpio export");
        return -1;
    }

    if (wiringPiSetupSys() == -1) {
        printf("Error initializing WiringPi\n");
        return -1;
    };

    num_packets_expected = 0;

    cout << "Initializing..." << endl;

    if (!manager.init()) {
        printf("Init failed\n");
    }

    // Max power level
    driver.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm);

    // Random channel
    driver.setChannel(RF_CHANNEL);

    //manager.setTimeout(DATAGRAM_TIMEOUT);
    
    //manager.setRetries(NUM_RETRIES);

    //driver.printRegisters();

    //uint8_t status = driver.statusRead();
    //printf("Status: %X\n", status);

    // forever loop
    while (!flag) {
        cout << "In while loop" << endl;
        // Send request: request(0), type 1 (temp) id = don't care, data = don't care)
        payload request_payload{0, 'T', 0, 0};

        bool result = manager.sendtoWait((uint8_t *) & request_payload, sizeof (request_payload), PUMP_ADDRESS);

        //uint8_t status = driver.statusRead();
        //printf("Status: %X\n", status);

        if (result == false) {
            cout << "Error sending request." << endl;
        } else {

            cout << "Request sent." << endl;

            num_packets_expected = 3;

            bool timeout = false;

            while (!timeout && num_packets_expected > 0) {

                cout << "Waiting for " << num_packets_expected << " packets." << endl;
                bool avail = manager.waitAvailableTimeout(DATAGRAM_TIMEOUT*3);

                if (avail && manager.available()) {
                    getAndPrintResponse();
                    num_packets_expected--;
                } else {
                    timeout = true;
                }
            }

            if (num_packets_expected > 0) {
                printf("%u response packets not received.\n", num_packets_expected);
            }


        }

    }

    system("gpio unexportall");

}

void getAndPrintResponse() {
    cout << "Get and print response" << endl;
    if (manager.available()) {
        payload response;
        uint8_t from;
        uint8_t len = sizeof (response);

        manager.recvfromAck((uint8_t *) & response, &len, &from);

        printf("Got response of size=%u. Contents:\n", len);
        printf("Response: %u | Type: %c | ID: %c | Value: %f\n", response.response, response.type, response.id, response.value);
    } else {
        printf("Manager not available; error\n");
        abort();
    }

}

void sig_handler(int sig) {
    printf("\nExiting...\n");
    flag = 1;
}
