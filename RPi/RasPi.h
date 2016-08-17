// RasPi.h
//
// Routines for implementing RadioHead on Raspberry Pi
// using BCM2835 library for GPIO
// Contributed by Mike Poublon and used with permission

#ifndef RASPI_h
#define RASPI_h

#include <string>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include "RadioHead.h"
#include "wiringPi.h"

#ifndef NULL
#define NULL 0
#endif

typedef unsigned char byte;

//#ifndef OUTPUT
//  #define OUTPUT BCM2835_GPIO_FSEL_OUTP
//#endif

using namespace std;

class SPIClass {
public:
    static byte transfer(byte _data);
    // SPI Configuration methods
    static void begin(); // Default
    static void begin(uint32_t frequency, uint8_t bitOrder, uint8_t dataMode);
    static void end();
    static void setBitOrder(uint8_t);
    static void setDataMode(uint8_t);
    static void setFrequency(uint32_t);
};

extern SPIClass SPI;

class SerialSimulator {
public:
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

    // TODO: move these from being inlined
    static void begin(int baud);
    static size_t println(const char* s);
    static size_t print(const char* s);
    static size_t print(unsigned int n, int base = DEC);
    static size_t print(char ch);
    static size_t println(char ch);
    static size_t print(unsigned char ch, int base = DEC);
    static size_t println(unsigned char ch, int base = DEC);
};

extern SerialSimulator Serial;

long random(long min, long max);

#endif
