/* 
 * File:   keypad.h
 * Author: michael
 *
 * Created on August 12, 2015, 6:55 PM
 */

#ifndef KEYPAD_H
#define	KEYPAD_H

#define COL1 3
#define COL2 4
#define COL3 5
#define COL4 6
#define ROW1 26
#define ROW2 27
#define ROW3 28
#define ROW4 29

// How long in milliseconds to wait before listening for another button
#define DEBOUNCE_TIME 300

// Priority -> realtime
#define MAX_PRIORITY  99

void runAndWaitForInput();

void registerKeypad();

void colOutRowIn();
void rowOutColIn();

void scanColumns();

void row1Isr();
void row2Isr();
void row3Isr();
void row4Isr();

#endif	/* KEYPAD_H */

