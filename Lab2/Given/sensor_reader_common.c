//
// This file contains poll/interrupt independent functions
// and constant definitions.
// It should not be compiled directly. Look at the polling
// and interrupt-based source files for compile instructions.


#include <sched.h> 	// For priority enhancement
#include <string.h> // For memset()
#include <stdio.h>	// For printf(), file operations
#include <stdint.h> // For uint8_t
#include <stdlib.h> // For system()
#include <stdbool.h> // For booleans
#include <time.h>	// For time functions

#include "wiringPi.h"	// For the WiringPi library
#include "sensor_reader.h" // Function declarations and constant definitions.

// Constant definitions
const int MAX_PRIORITY = 99;
const int SENSOR_PIN_NUM = 4;
const int READ_INIT_DELAY_MS = 20;	
const int WAIT_FOR_SENSOR_DELAY_MAX_US= 40;	
const int RESPONSE_TIME_US = 80;
const int PRE_BIT_DELAY_US = 50;
const int MAX_TIME_FOR_ZERO_BIT_US = 28;
const int MAX_TIME_FOR_ONE_BIT_US = 70;
const int MAX_TIME_BUFFER = 10;
const int HUMID_INT_OFFSET = 0;
const int HUMID_DEC_OFFSET = 8;
const int TEMP_INT_OFFSET = 16;
const int TEMP_DEC_OFFSET = 24;
const int CHECKSUM_OFFSET = 32;
const int BITS_PER_BYTE = 8;
const int ERROR = -1;
const int MAX_NUM_READS = 100;
const int ONE_SEC_IN_US = 1000000;
const char * const OUTPUT_FILENAME = "eecs317Group4SensorReadings.txt";

// Global Initializations
bool readSuccessful = false;

// We use debug mode to print timings associated with the 
// measurements.
//#define DEBUG_MODE
#ifdef DEBUG_MODE
	int numUsWaitingForSensorResponse = 0;
	int numUsLowSensorResponse = 0;
	int numUsHighSensorResonse = 0;
	int numUsLowPreBit[TOTAL_BITS_PER_READ];
	int numUsForBit[TOTAL_BITS_PER_READ];
	int bitNum = 0;
#endif

/*
 * Initializes the GPIO pin to high
 */
void setupGpio()
{
	// Call setup
	wiringPiSetupGpio();
	
	// Reserve the GPIO pin
	pinMode(SENSOR_PIN_NUM, OUTPUT);
	
	// Set the line high by default
	digitalWrite(SENSOR_PIN_NUM, HIGH);
	
	pullUpDnControl(SENSOR_PIN_NUM, PUD_UP);
}

// Releases the GPIO reservation.
void releaseGpio()
{
	system("gpio-admin export 4");
	system("gpio-admin unexport 4");
}

/*
 * Returns the 8-bit integer whose first bit is located at
 * at bits_rcvd[offset]
 */
int arrAndOffsetToInt(int * bits_rcvd, int offset)
{
	int val = 0;
	int bit_idx = 0;
	for (; bit_idx < BITS_PER_BYTE; ++bit_idx)
	{
		val |= bits_rcvd[offset + 7 - bit_idx] << bit_idx;
	}
	return val;
}

/*
 * Generates a checksum from the humidity and temp readings.
 */
int generateChecksum(int * bits_rcvd)
{
	// Use uint8_t variables to ensure that the result of each addition
	// is only eight bits.
	uint8_t humid_int = arrAndOffsetToInt(bits_rcvd, HUMID_INT_OFFSET);
	uint8_t humid_dec = arrAndOffsetToInt(bits_rcvd, HUMID_DEC_OFFSET);
	uint8_t temp_int = arrAndOffsetToInt(bits_rcvd, TEMP_INT_OFFSET);
	uint8_t temp_dec = arrAndOffsetToInt(bits_rcvd, TEMP_DEC_OFFSET);
	
	uint8_t checksum = humid_int + humid_dec + temp_int + temp_dec;
	return checksum;
}

/*
 * Processes the data read, and writes to a file in the event that
 * the reading was successful.
 */
void analyzeAndPrintResults(int * bitsRcvd, const char * errorString, const char * sensorInteractionMode)
{	
	if (errorString != NULL)
	{
		printf("%s", errorString);
	}

	char * timeAsString = getTimeAsString();
	
	if (timeAsString != NULL)
	{
		printf("Time of reading: %s", timeAsString);
	}
	
	// Parse the recieved bits
	int humid_int = arrAndOffsetToInt((int *)bitsRcvd, HUMID_INT_OFFSET);
	int humid_dec = arrAndOffsetToInt((int *)bitsRcvd, HUMID_DEC_OFFSET);
	int temp_int = arrAndOffsetToInt((int *)bitsRcvd, TEMP_INT_OFFSET);
	int temp_dec = arrAndOffsetToInt((int *)bitsRcvd, TEMP_DEC_OFFSET);
	int checksum_read = arrAndOffsetToInt((int *)bitsRcvd, CHECKSUM_OFFSET);
	int checksum_generated = generateChecksum((int *)bitsRcvd);
	
	if (checksum_read == checksum_generated && humid_int > 0 && temp_int > 0)
	{
		readSuccessful = true;
	}
	
	// Print the results
	printf("Temp: %d.%d\n", temp_int, temp_dec);
	printf("Humidity: %d.%d\n", humid_int, humid_dec);
	printf("Checksum valid: %s\n", checksum_read == checksum_generated ? "true" : "false");
	printf("Checksum read: %d\n", checksum_read);
	printf("Checksum calc'd: %d\n", checksum_generated);
	
	// Write the results to a file
	writeResultsToFile(temp_int, temp_dec, humid_int, humid_dec, checksum_read, checksum_generated, sensorInteractionMode, timeAsString, errorString);
}

/*
 * Write the temperature, humidity, and the sensor communcation type to
 * a file. The filename is the constant OUTPUT_FILENAME. It appends to
 * the file rather than overwrites it (if it exists already). The file
 * will be searched for in the current working directory.
 */
void writeResultsToFile(int temp_int, int temp_dec, 
						int humid_int, int humid_dec,
						int checksumRead, int checksumGen,
						const char * sensorInteractionMode,
						const char * timeAsString,
						const char * errorString)
{
	// Open the file
	FILE * outFp = fopen(OUTPUT_FILENAME, "a");
	
	if (outFp == NULL)
	{
		printf("\nERROR WRITING DATA TO FILE!\n");
	}
	
	fprintf(outFp, "\n\n\nRESULTS OF READING VIA: %s\n", sensorInteractionMode);
	
	if (errorString != NULL)
	{
		fprintf(outFp, "%s", errorString);
	}
	
	if (timeAsString == NULL)
	{
		fprintf(outFp, "ERROR RETRIEVING TIME\n");
	}
	else
	{
		fprintf(outFp, "TIME OF MEASUREMENT: %s", timeAsString);
	}
	
	// Print the results
	fprintf(outFp, "Temp: %d.%d\n", temp_int, temp_dec);
	fprintf(outFp, "Humidity: %d.%d\n", humid_int, humid_dec);
	fprintf(outFp, "Checksum read: %d\n", checksumRead);
	fprintf(outFp, "Checksum calc'd: %d\n", checksumGen);
	fprintf(outFp, "Checksum valid: %s\n", checksumRead == checksumGen ? "true" : "false");
	
	if(fclose(outFp) != 0)
	{
		printf("\nERROR CLOSING FILE!\n");
	}
}

/*
 * Gets the current time/date as string.
 */
char * getTimeAsString()
{
	// Get the time and write it to the file.
	time_t currentTime;
	char * timeAsString;
	currentTime = time(NULL);
	if (currentTime == ((time_t)-1))
	{
		printf("ERROR ACCESSING TIME!\n");
	}
	
	timeAsString = ctime(&currentTime);
	if (timeAsString == NULL)
	{
		printf("ERROR CONVERTING TIME TO STRING!\n");
	}
	
	return timeAsString;
}

/*
 * piHiPri:
 *	Attempt to set a high priority schedulling for the running program
 *********************************************************************************
 */

int piHiPri (const int pri)
{
  struct sched_param sched ;

  memset (&sched, 0, sizeof(sched)) ;

  if (pri > sched_get_priority_max (SCHED_RR))
    sched.sched_priority = sched_get_priority_max (SCHED_RR) ;
  else
    sched.sched_priority = pri ;

  return sched_setscheduler (0, SCHED_RR, &sched) ;
}
