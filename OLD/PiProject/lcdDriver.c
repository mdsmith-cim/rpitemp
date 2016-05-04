#include <wiringPi.h>

#include "lcdDriver.h"

int fd = -1;
char backlight = 0;

/* Pinout:
0x80    P7 -  - D7     
0x40    P6 -  - D6
0x20    P5 -  - D5
0x10    P4 -  - D4    
    -----------
0x08    P3 -  - BL   Backlight
0x04    P2 -  - EN   Starts Data read/write 
0x02    P1 -  - RW   low: write, high: read   
0x01    P0 -  - RS   Register Select: 0: Instruction Register (IR) (AC when read), 1: data register (DR)
 

 * */

// Private func
int send(int data);
int lowLevelReset();

int LCDinit() {
    fd = wiringPiI2CSetup(0x3F);
    if (fd == -1) {
        return INIT_ERR;
    }

    int result = lowLevelReset();

    // Program degree symbol - set CGRAM addr 0x01
//    result += sendCmd(SET_CGRAM_ADDR | 0x01);
//    result += sendChar(0x7);
//    result += sendChar(0x5);
//    result += sendChar(0x7);
//    result += sendChar(0x0);
//    result += sendChar(0x0);
//    result += sendChar(0x0);
//    result += sendChar(0x0);
//    result += sendChar(0x0);


    // Set display address to 0 so future data is displayed
    result += sendCmd(SET_DISP_ADDR);

    // Send commands...
    result += sendCmd(DISPLAY_ON_OFF_CTL | DOFC_DISP_ON);
    result += clearDisplay();

    result += setBacklight(1);

    return result;
}

// Perform display complete reset according to datasheet
// This includes the delays

int lowLevelReset() {
    int result = wiringPiI2CWrite(fd, 0x30 | ENABLE); //0x30 = 11 for DB5 - DB4, 0 for all other data and RS + R/W
    result += wiringPiI2CWrite(fd, 0x30 & ~ENABLE);
    delay(5);
    result += wiringPiI2CWrite(fd, 0x30 | ENABLE);
    result += wiringPiI2CWrite(fd, 0x30 & ~ENABLE);
    delayMicroseconds(105);
    result += wiringPiI2CWrite(fd, 0x30 | ENABLE);
    result += wiringPiI2CWrite(fd, 0x30 & ~ENABLE);
    delayMicroseconds(40);
    result += wiringPiI2CWrite(fd, 0x20 | ENABLE); //0x20= DB5 = 1, DB4 = 0, else same as 0x30
    result += wiringPiI2CWrite(fd, 0x20 & ~ENABLE);
    delayMicroseconds(40);

    result += wiringPiI2CWrite(fd, 0x20 | ENABLE);
    result += wiringPiI2CWrite(fd, 0x20 & ~ENABLE);
    result += wiringPiI2CWrite(fd, (0b1000 << 4) | ENABLE); // Func Set N + F
    result += wiringPiI2CWrite(fd, (0b1000 << 4) & ~ENABLE);
    delayMicroseconds(40);

    result += wiringPiI2CWrite(fd, 0 | ENABLE);
    result += wiringPiI2CWrite(fd, 0 & ~ENABLE);
    result += wiringPiI2CWrite(fd, 0b1000 | ENABLE);
    result += wiringPiI2CWrite(fd, 0b1000 & ~ENABLE);
    delayMicroseconds(40);

    result += wiringPiI2CWrite(fd, 0 | ENABLE);
    result += wiringPiI2CWrite(fd, 0 & ~ENABLE);
    result += wiringPiI2CWrite(fd, 0b0001 | ENABLE);
    result += wiringPiI2CWrite(fd, 0b0001 & ~ENABLE);
    delayMicroseconds(40);

    result += wiringPiI2CWrite(fd, 0 | ENABLE);
    result += wiringPiI2CWrite(fd, 0 & ~ENABLE);
    result += wiringPiI2CWrite(fd, 0b0110 | ENABLE); // Increment, no shift
    result += wiringPiI2CWrite(fd, 0b0110 & ~ENABLE);
    delayMicroseconds(40);
    return result;
}

// To use: data variable has 8 bits - the command
// Will not support reading of data

int sendCmd(char cmd) {
    return send((WRITE_COMMAND << 8) | cmd);
}

// To use: data variable has 8 bits - the character
// Will not support reading of data

int sendChar(char data) {
    return send((WRITE_DATA << 8) | data);
}

int sendString(const char* string, int row) {

    int length = strlen(string);

    if (row < 0 || row > LCD_HEIGHT) {
        row = 1;
    }

    if (length > LCD_WIDTH) {
        length = LCD_WIDTH;
    }
    int result = 0;
    // Set location based on row
    sendCmd(SET_DISP_ADDR | 0x40 * (row - 1));

    for (int i = 0; i < length; i++) {
        result += sendChar(string[i]);
    }
    return result;
}

int clearDisplay() {
    int result = sendCmd(CLR_DISPLAY);
    delay(2);
    return result;
}

// Data is sent in 4 bit mode....
// R/W RS D7 D6 D5 D4 D3 D2 D1 D0

int send(int data) {

    // Send MSB 4 bits first, including enable signal
    int data1 = (data & 0xF0) | backlight | ENABLE | (data >> 8);
    int result = wiringPiI2CWrite(fd, data1);
    // Remove enable signal
    result += wiringPiI2CWrite(fd, data1 & ~ENABLE);

    // Send LSB 4 bits, with enable signal
    int data2 = ((data & 0xF) << 4) | backlight | ENABLE | (data >> 8);
    result += wiringPiI2CWrite(fd, data2);

    // Remove enable signal
    result += wiringPiI2CWrite(fd, data2 & ~ENABLE);
    
    // Give LCD some time to process
    // Minimum of 40us; we wait a little longer to avoid using wiringPi's
    // hardcoded delay loop
    delayMicroseconds(105);

    return result;
}


// 1 = on, 0 = off

int setBacklight(char setting) {
    // Update variable for future commands
    backlight = setting << 3;
    // Send backlight request to chip; first, get current pin readings
    int current = wiringPiI2CRead(fd);
    current &= 0x4;
    // Modify current settings with new backlight
    int result = wiringPiI2CWrite(fd, current | backlight);
    delayMicroseconds(40);
    return result;

}