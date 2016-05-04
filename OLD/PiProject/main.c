/* 
 * File:   main.c
 * Author: michael
 *
 * Created on August 14, 2015, 2:29 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "keypad.h"
#include "lcdDriver.h"
#include "getTemp.h"
#include <time.h>
#include "termios.h"
#include "fcntl.h"

// Private function definitions
void term_reset();
void term_nonblocking();

/*
 * 
 */
int main(int argc, char** argv) {
    // Reset LCD
    printf("Resetting LCD...\n");
    if (LCDinit() != 0) {
        return EXIT_FAILURE;
    }

    // Intialize wiringPi for GPIO use for sensor
    printf("Initializing wiringPi library...\n");
    if (wiringPiSetup() != 0) {
        return EXIT_FAILURE;
    }

    float temp, humid;
    int key = -1;

    // Get current time to create CSV file
    time_t t = time(NULL);
    struct tm * p = localtime(&t);
    char currentTime[512];
    if (strftime(currentTime, 512, "%F-%r", p) == 0) {
        return EXIT_FAILURE;
    }

    // Open CSV file
    char filename[512];
    sprintf(filename, "temp_humidity_log-%s.csv", currentTime);
    printf("Opening CSV file: %s\n", filename);

    FILE *fp;
    fp = fopen(filename, "w");
    fprintf(fp, "Date and Time,Temperature,Humidity\n");

    // Allow non-blocking key read
    term_nonblocking();

    printf("Running...press any key to stop recording and exit.\n");
    while (key == -1) {

        // Get temperature, humidity from sensor, if valid, display/save
        if (readTemp(DHT_DATA_PIN, &temp, &humid) == 0) {

            // Format data for LCD
            char line1[16], line2[16];
            sprintf(line1, "Temp: %.1fC", temp);
            sprintf(line2, "Humid: %.1f%%", humid);
            // Send to LCD
            sendString(line1, 1);
            sendString(line2, 2);
            
            // Create one line entry for CSV
            char dataLine[512];

            // Get time for entry
            t = time(NULL);
            p = localtime(&t);

            if (strftime(dataLine, 512, "%F %T", p) == 0) {
                printf("Unable to format time. Exiting...\n");
                fclose(fp);
                return EXIT_FAILURE;
            }

            sprintf(dataLine, "%s,%.1f,%.1f\n", dataLine,temp,humid);

            // Write to CSV file
            fprintf(fp, "%s", dataLine);

        }
        sleep(2);

        key = getchar();
    }

    fclose(fp);
    return EXIT_SUCCESS;
}

// Code for allowing non-blocking user input to allow exiting the loop
// From http://stackoverflow.com/a/16938355
struct termios stdin_orig; // Structure to save parameters

void term_reset() {
    tcsetattr(STDIN_FILENO, TCSANOW, &stdin_orig);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &stdin_orig);
}

void term_nonblocking() {
    struct termios newt;
    tcgetattr(STDIN_FILENO, &stdin_orig);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // non-blocking
    newt = stdin_orig;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    atexit(term_reset);
}

