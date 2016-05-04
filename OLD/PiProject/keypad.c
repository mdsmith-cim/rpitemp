/* 
 * File:   keypad.c
 * Author: michael
 *
 * Created on August 12, 2015, 6:51 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "keypad.h"
#include "wiringPi.h"
#include <unistd.h>


volatile int rowPressed;
volatile int wasKeyPressed;
volatile char keyChar;
volatile long lastTimeRowDetected;


void runAndWaitForInput() {
    wiringPiSetup();

    //Request high priority
    int prioStatus = piHiPri(MAX_PRIORITY);
    if (prioStatus != 0) {
        printf("Unable to set priority!\n");
    }

    // Initialize variables
    rowPressed = 0;
    wasKeyPressed = 0;
    keyChar = ' ';
    lastTimeRowDetected = 0;

    registerKeypad();

    while (1) {
        piLock(0);
        if (wasKeyPressed) {
            wasKeyPressed = 0;
            printf("New key: %c\n", keyChar);
            if (keyChar == '*') {
                return;
            }
        }
        piUnlock(0);
        delay(300);
    }

}

void registerKeypad() {

    // Set row as input, column output
    colOutRowIn();
    // Set up interrupts on rows
    wiringPiISR(ROW1, INT_EDGE_FALLING, &row1Isr);
    wiringPiISR(ROW2, INT_EDGE_FALLING, &row2Isr);
    wiringPiISR(ROW3, INT_EDGE_FALLING, &row3Isr);
    wiringPiISR(ROW4, INT_EDGE_FALLING, &row4Isr);
}

void colOutRowIn() {
    //Set rows as input
    pinMode(ROW1, INPUT);
    pinMode(ROW2, INPUT);
    pinMode(ROW3, INPUT);
    pinMode(ROW4, INPUT);

    //Enable row pull-ups
    pullUpDnControl(ROW1, PUD_UP);
    pullUpDnControl(ROW2, PUD_UP);
    pullUpDnControl(ROW3, PUD_UP);
    pullUpDnControl(ROW4, PUD_UP);

    // Set columns as output
    pinMode(COL1, OUTPUT);
    pinMode(COL2, OUTPUT);
    pinMode(COL3, OUTPUT);
    pinMode(COL4, OUTPUT);

    // Set output to 0
    digitalWrite(COL1, LOW);
    digitalWrite(COL2, LOW);
    digitalWrite(COL3, LOW);
    digitalWrite(COL4, LOW);
}

void rowOutColIn() {
    //Set columns as input
    pinMode(COL1, INPUT);
    pinMode(COL2, INPUT);
    pinMode(COL3, INPUT);
    pinMode(COL4, INPUT);

    //Enable column pull-ups
    pullUpDnControl(COL1, PUD_UP);
    pullUpDnControl(COL2, PUD_UP);
    pullUpDnControl(COL3, PUD_UP);
    pullUpDnControl(COL4, PUD_UP);

    // Set rows as output
    pinMode(ROW1, OUTPUT);
    pinMode(ROW2, OUTPUT);
    pinMode(ROW3, OUTPUT);
    pinMode(ROW4, OUTPUT);

    // Set output to 0
    digitalWrite(ROW1, LOW);
    digitalWrite(ROW2, LOW);
    digitalWrite(ROW3, LOW);
    digitalWrite(ROW4, LOW);
}

void scanColumns() {
    // Change rows to output
    rowOutColIn();
    // Read columns and see what we have
    int col1Value = digitalRead(COL1);
    int col2Value = digitalRead(COL2);
    int col3Value = digitalRead(COL3);
    int col4Value = digitalRead(COL4);

    // Determine what was pressed...
    if (col1Value == 0) {
        switch (rowPressed) {
            case ROW1:
                keyChar = 'D';
                break;
            case ROW2:
                keyChar = 'C';
                break;
            case ROW3:
                keyChar = 'B';
                break;
            case ROW4:
                keyChar = 'A';
                break;
        }

    } else if (col2Value == 0) {
        switch (rowPressed) {
            case ROW1:
                keyChar = '#';
                break;
            case ROW2:
                keyChar = '9';
                break;
            case ROW3:
                keyChar = '6';
                break;
            case ROW4:
                keyChar = '3';
                break;
        }
    } else if (col3Value == 0) {
        switch (rowPressed) {
            case ROW1:
                keyChar = '0';
                break;
            case ROW2:
                keyChar = '8';
                break;
            case ROW3:
                keyChar = '5';
                break;
            case ROW4:
                keyChar = '2';
                break;
        }
    } else if (col4Value == 0) {
        switch (rowPressed) {
            case ROW1:
                keyChar = '*';
                break;
            case ROW2:
                keyChar = '7';
                break;
            case ROW3:
                keyChar = '4';
                break;
            case ROW4:
                keyChar = '1';
                break;
        }
    } else {
        // In case we can't properly determine the key e.g. really fast key press
        keyChar = '!';
    }

    if (keyChar != '!') {
        piLock(0);
        wasKeyPressed = 1;
        piUnlock(0);
    }
    // Reset to default state
    colOutRowIn();
}

inline void genericISR(int row) {
    unsigned int curTime = millis();
    long diff = (long)millis() - lastTimeRowDetected;
    if (diff > DEBOUNCE_TIME || diff < 0) {
        lastTimeRowDetected = (long)curTime;
        rowPressed = row;
        scanColumns();
    }
}

void row1Isr() {
    genericISR(ROW1);

}

void row2Isr() {
    genericISR(ROW2);
}

void row3Isr() {
    genericISR(ROW3);
}

void row4Isr() {
    genericISR(ROW4);
}