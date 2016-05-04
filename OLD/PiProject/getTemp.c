#include "getTemp.h"

// For the filter..
#define DecayFactor 0.95 //DecayFactor = exp(-2.0 * M_PI * CutoffFrequency / SampleRate);
#define AmplitudeFactor (1.0 - DecayFactor)
// Temp is index 0, humidity is index 1
float MovingAverage[] = {21,50}; // Initialize to sane constants

// For internal use to this file only
float lowpassFilter(float newInput, int type);

int readTemp(int pin, float* temperature, float* humidity) {

    // Exit if bad arguments
    if (humidity == NULL || temperature == NULL) {
        return ERROR_ARGUMENT;
    }

    // Init variables to 0
    *temperature = 0.0f;
    *humidity = 0.0f;

    // This will store the time at which each rising/falling edge of the signal 
    // is detected
    unsigned int changeTime[NUM_BITS * 2];
    int arrayIndex = 0;

    //Request high priority
    int prioStatus = piHiPri(MAX_PRIORITY);
    if (prioStatus != 0) {
        return PRIORITY_ERR;
    }

    // Set output high for 500ms
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    delay(500);

    // Set low for 1ms
    digitalWrite(pin, LOW);
    delayMicroseconds(1000);

    //Set input = bring it high, wait for data from sensor
    pinMode(pin, INPUT);

    // Wait a very short while to make sure it goes high after mode change
    // Otherwise, when we wait for it to go low it may yet have not gone high
    delayMicroseconds(10);

    //Wait for it to go low; max 50us
    unsigned int curTime = micros();
    while (digitalRead(pin)) {
        if (micros() - curTime > T_go) {
            return T_GO_ERR;
        }
    }

    //Wait for it to go high, max 85us (a slightly long '1' is sent as a first bit)
    curTime = micros();
    while (!digitalRead(pin)) {
        if (micros() - curTime > T_rel) {
            return T_REL_ERR;
        }
    }

    // Now get all data
    int prevValue = 1; // We know this to be fact
    unsigned int startTime = millis();
    while (1) {
        // Get pin value, constantly poll b/c that's the only way to do it fast
        int curValue = digitalRead(pin);
        // If there's a change
        curTime = micros();
        if (curValue != prevValue) {
            changeTime[arrayIndex++] = curTime;
            // Record change
            prevValue = curValue;
            // Exit if done
            if (arrayIndex > NUM_BITS * 2) {
                break;
            }
        }// Break out of loop if we've been sitting here too long
        else if (millis() - startTime > TIMEOUT_READ) {
            return TIMEOUT_ERR;
        }
    }

    //Request normal priority, ignore any errors
    piHiPri(DEFAULT_PRIORITY);

    // Determine a scale factor to compensate for the delay of the Raspberry pi
    // We measure how long the line is held low, and compare that to the real value of 50us
    float scaleFactor = 0;
    for (int i = 1; i < NUM_BITS * 2; i += 2) {
        unsigned int time = changeTime[i] - changeTime[i - 1];
        scaleFactor += (float) time;
    }

    scaleFactor = (scaleFactor / NUM_BITS) / T_LOW;

    // Get data
    // Note that some of the below code is based off of or taken from 
    // https://github.com/adafruit/Adafruit_Python_DHT
    unsigned char data[5] = {0};
    for (int i = 2; i < NUM_BITS * 2; i += 2) {
        // This is how long the line is high
        // By the datasheet, if it is held high for a short time = 0
        // Long time = 1
        unsigned int time = changeTime[i] - changeTime[i - 1];

        // Write the appropriate bit
        int index = (i - 2) / 16;
        data[index] <<= 1;
        if (time > MEAN_HI_LOW * scaleFactor) {
            data[index] |= 1;
        }

    }

    // If bad checksum, exit
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return CHECKSUM_ERR;
    } else {
        float internalTemp, internalHumid;
        internalHumid = (data[0] * 256 + data[1]) / 10.0f;
        internalTemp = ((data[2] & 0x7F) * 256 + data[3]) / 10.0f;
        if (data[2] & 0x80) {
            internalTemp *= -1.0f;
        }
        // One last check: it is possible the checksum may not catch all errors
        // So if humidity or temp outside acceptable range, return error
        // Note that we are considering range to within precision, so for ex.
        // humidity max = 100 but w/in 5%
        if (internalHumid < -5 || internalHumid > 105 || internalTemp < -40.5 || internalTemp > 80.5) {
            return OUTSIDE_RANGE_ERR;
        }
        *temperature = lowpassFilter(internalTemp, 0);
        *humidity = lowpassFilter(internalHumid, 1);
    }
    return SUCCESS;
}

// Lowpass filter to get rid of erroneous +50C / high humidity values
// Note there is no error checking i.e. bad type...
float lowpassFilter(float newInput, int type)
{
   MovingAverage[type] *= DecayFactor;
   MovingAverage[type] += AmplitudeFactor * newInput;

   return MovingAverage[type];
}
