
#include <stdio.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define	DHT_DATA_PIN	0 // 6 from right, lower

// Priority -> realtime
#define MAX_PRIORITY  99
#define DEFAULT_PRIORITY 1
// Error code / success definitions
#define SUCCESS 0
#define CHECKSUM_ERR 1
#define ERROR_ARGUMENT 2
#define T_GO_ERR  3
#define T_REL_ERR  4
#define TIMEOUT_ERR 5
#define PRIORITY_ERR 6
#define WIRINGPI_SETUP_ERR 7
#define OUTSIDE_RANGE_ERR 8

// Number of bits 
#define NUM_BITS  41
// Timeout to read data
#define TIMEOUT_READ 5 //ms
// From datasheet, maximum time to wait for various states + a little longer
#define T_LOW 65 //us
#define MEAN_HI_LOW 48 //us
#define T_go 250 //us
#define T_rel 100 //us

// Colors for printing
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

int readTemp(int pin, float* temperature, float* humidity);