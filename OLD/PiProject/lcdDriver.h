/* 
 * File:   lcdDriver.h
 * Author: michael
 *
 * Created on August 14, 2015, 2:57 PM
 */

#ifndef LCDDRIVER_H
#define	LCDDRIVER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <wiringPiI2C.h>

#define INIT_ERR -2

    /***** Data codes for various commands *****/

    // Definitions for commands with only 1 possible value
#define CLR_DISPLAY 0b1
#define RETURN_HOME 0b10

    //Definitions for commands with different possible values
#define ENTRY_MODE_SET_GENERAL 0b100
#define EMS_INCREMENT 0b10
#define EMS_SHIFT 0b1

#define DISPLAY_ON_OFF_CTL 0b1000
#define DOFC_DISP_ON 0b100
#define DOFC_CURSOR_ON 0b10
#define DOFC_CURSOR_BLINK 0b1

#define CURSOR_OR_DISP_SHIFT 0b10000
#define CODS_DISP_SHIFT 0b1000
#define CODS_SHIFT_RIGHT 0b100

#define FUNCTION_SET 0b100000
#define FS_DL_8BITS 0b10000
#define FS_2_LINES 0b1000
#define FS_5x10_DOT 0b100

#define SET_DISP_ADDR 0b10000000
    
#define SET_CGRAM_ADDR 0b01000000


    /***** END Data codes for various commands END *****/

    /*** General definitions, e.g. command vs data*/
#define WRITE_COMMAND 0b0 //RS. R/W = 0
#define WRITE_DATA 0b1// Rs = 1, R/W = 0
#define READ_BUSY_AND_ADDR 0b10 // RS = 0, R/W = 1
#define ENABLE 0b100
    
#define LCD_WIDTH 16
#define LCD_HEIGHT 2

    int LCDinit();
    int sendCmd(char cmd);
    int sendChar(char data);
    int sendString(const char* string, int row);
    int setBacklight(char setting);
    int clearDisplay();

#ifdef	__cplusplus
}
#endif

#endif	/* LCDDRIVER_H */

