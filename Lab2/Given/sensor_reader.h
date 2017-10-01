// 
// 	This header contains function declarations and
// 	constant declarations associated with the sensor reading
// 	code.

#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <stdbool.h>

// Constants - Defined in sensor_reader_common.c
#define TOTAL_BITS_PER_READ 40
extern const int MAX_PRIORITY = 99;
extern const int SENSOR_PIN_NUM;
extern const int READ_INIT_DELAY_MS;	
extern const int WAIT_FOR_SENSOR_DELAY_MAX_US;	
extern const int RESPONSE_TIME_US;
extern const int PRE_BIT_DELAY_US;
extern const int MAX_TIME_FOR_ZERO_BIT_US;
extern const int MAX_TIME_FOR_ONE_BIT_US;
const int MAX_TIME_BUFFER;
extern const int HUMID_INT_OFFSET;
extern const int HUMID_DEC_OFFSET;
extern const int TEMP_INT_OFFSET;
extern const int TEMP_DEC_OFFSET;
extern const int CHECKSUM_OFFSET;
extern const int BITS_PER_BYTE;
extern const int ERROR;
extern const int MAX_NUM_READS;
extern const int ONE_SEC_IN_US;
extern const char * const OUTPUT_FILENAME;

// Global Variables
extern bool readSuccessful;

// Function Declarations - Common
int piHiPri(const int pri);
void setupGpio();
void releaseGpio();
void takeMeasurement();
void setupRead(); 
int receiveBit();
int arrAndOffsetToInt(int * bits_rcvd, int offset);
int generateChecksum(int * bits_rcvd);
void writeResultsToFile(int temp_int, int temp_dec, 
						int humid_int, int humid_dec,
						int checksumRead, int checksumGen,
						const char * sensorInteractionMode,
						const char * timeAsString,
						const char * errorString);
						
char * getTimeAsString();
void analyzeAndPrintResults(int * bitsRcvd, const char * errorString, const char * sensorInteractionMode);

// Function Declarations - interrupt-based only
__attribute__((always_inline)) inline void initiateRead();
__attribute__((always_inline)) inline void sensorReadISR();

// Enum describing current state. For interrupt-based approach.
typedef enum State
{
	INIT_PULL_LINE_LOW,
	INPUT_JUST_ENABLED,
	HIGH_ACK,
	BIT_READ_RISING,
	READ_COMPLETE,
	ERROR_STATE
} State;

#endif //SENSOR_READER_H
