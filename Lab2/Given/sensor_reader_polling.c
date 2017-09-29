// This file contains the polling-specific functions.
// To compile: gcc sensor_reader_common.c sensor_reader_polling.c -o sensor_reader_polling -lwiringPi -Wall

#include <stdio.h>	// For printf()
#include <string.h>	// For memset()

#include "wiringPi.h"	// For the WiringPi library
#include "sensor_reader.h" // Function declarations and constant definitions.

int main()
{
	// Set this process to have maximum priority.
	if (piHiPri(MAX_PRIORITY) == ERROR)
	{
		printf("Error setting priority! Exiting");
		return ERROR;
	}
	
	// Attempt MAX_NUM_READS measurements in 1-second intervals.
	// Stop if a successful measurement has been taken.
	int readIdx = 0;
	while (!readSuccessful && readIdx < MAX_NUM_READS)
	{
		printf("\nSample Number: %i\n", readIdx);
		setupGpio();
		takeMeasurement();
		delayMicroseconds(ONE_SEC_IN_US);
		++readIdx;
	}
	
	// Free the GPIO
	releaseGpio();
	
	return 0;
}

/*
 * Takes a measurement. This function will read 40 bits
 * and print the humidity, temperature, and checksum
 * associated with this measurement. If an error occurs,
 * a warning will be printed.
 */
void takeMeasurement()
{
	// Prepare an array to hold the rcvd data
	int bitsRcvd[TOTAL_BITS_PER_READ];
	memset(bitsRcvd, ERROR, TOTAL_BITS_PER_READ * sizeof(int));
	
	bool errorOccurred = false;
	
	// Tell the sensor that we want to take a measurement.
	setupRead();

	#ifndef DEBUG_MODE
		int bitNum = 0;
	#endif
	
	// Read 40 bits. Print an error and return
	// if there was an error reading.
	for (; bitNum < TOTAL_BITS_PER_READ; ++bitNum)
	{
		bitsRcvd[bitNum] = receiveBit();
		if (bitsRcvd[bitNum] == ERROR)
		{
			errorOccurred = true;
		}
	}

	// Write results to console and output.
	if (errorOccurred)
	{
		// Read should be done. Analyze the results.
		analyzeAndPrintResults(bitsRcvd, "Error reading\n", "polling");
	}
	else
	{
		analyzeAndPrintResults(bitsRcvd, NULL, "polling");
	}
		
	#ifdef DEBUG_MODE
		printf("Num us of waiting for the line to be pulled low pos start: %d\n", numUsWaitingForSensorResponse);
		printf("Num us waiting when the line is line is low, acking: %d\n", numUsLowSensorResponse);
		printf("Num us waiting when the line is line is high, acking: %d\n", numUsHighSensorResonse);
		int bit = 0;
		for (; bit < TOTAL_BITS_PER_READ; ++bit)
		{
			printf("Bit Num %d: Low wait: %d; High wait %d\n", bit, numUsLowPreBit[bit], numUsForBit[bit]); 
		}
		bitNum = 0;
	#endif
}

/* Reads a single bit from the SENSOR_PIN_NUM pin.
 * Precondition: The GPIO pin is in input mode
 *  and the read process has been initiated.
 */
int receiveBit()
{
	int retVal = ERROR;
	
	// First, wait at most 50uS before
	// the sensor transmits a bit.
	int numMicrosDelay = 0;
	while (digitalRead(SENSOR_PIN_NUM) == LOW && numMicrosDelay < PRE_BIT_DELAY_US)
	{
		delayMicroseconds(1);
		++numMicrosDelay;
	}
	if (numMicrosDelay >= PRE_BIT_DELAY_US)
	{
		printf("ERROR READING SENSOR! Pre-recv delay too high\n");
		return ERROR;
	}
	#ifdef DEBUG_MODE
		numUsLowPreBit[bitNum] = numMicrosDelay;
	#endif
	
	// Now, the sensor should pull the line high.
	// If the line is high for between 26 and 28uS,
	// the sensor is transmitting logical zero. If the line
	// is high for 70uS, a logical one has been transmitted.
	numMicrosDelay = 0;
	while (digitalRead(SENSOR_PIN_NUM) == HIGH && numMicrosDelay < MAX_TIME_FOR_ONE_BIT_US)
	{
		delayMicroseconds(1);
		++numMicrosDelay;
	}
	// A zero has been transmitted
	if (numMicrosDelay <= MAX_TIME_FOR_ZERO_BIT_US)
	{
		retVal=0;
	}
	// A one has been transmitted	
	else if (numMicrosDelay < MAX_TIME_FOR_ONE_BIT_US)
	{
		retVal=1;
	}
	// Error
	else
	{
		printf("ERROR READING SENSOR! Bit-level signal held too long\n");
		retVal=ERROR;
	}	
	#ifdef DEBUG_MODE
		numUsForBit[bitNum] = numMicrosDelay;
	#endif
	
	return retVal;
}


/* Manages the first section of the read.
 * Ties the line low for 20ms, then waits
 * the 80uS for both the high and low sections
 * of the sensor response
 * Postcondition: GPIO pin is in input mode.
 */
void setupRead()
{
	// This MUST be done before the line
	// is pulled low by the pi. Otherwise,
	// timing gets thrown off and will
	// ensure that the reading is BAD.
	pullUpDnControl(SENSOR_PIN_NUM, PUD_UP);

	// Pull the data line down for 20ms,
	// then pull it back up. The datasheet
	// says 500us, but this doesn't work.
	digitalWrite(SENSOR_PIN_NUM, LOW);
	delay(READ_INIT_DELAY_MS);
	digitalWrite(SENSOR_PIN_NUM, HIGH);

	// Change the pin the input mode.
	pinMode(SENSOR_PIN_NUM, INPUT);
	
	// Poll for the 20-40ms period
	int numMicrosDelay = 0;
	
	// This will likely only execute for several uS, not 20-40.
	while (digitalRead(SENSOR_PIN_NUM) == HIGH && numMicrosDelay < WAIT_FOR_SENSOR_DELAY_MAX_US)
	{
		delayMicroseconds(1);
		++numMicrosDelay;
	}
	if (numMicrosDelay >= WAIT_FOR_SENSOR_DELAY_MAX_US)
	{
		printf("ERROR READING SENSOR! Over 40us of waiting\n");
		return;
	}
	#ifdef DEBUG_MODE
		numUsWaitingForSensorResponse = numMicrosDelay;
	#endif 
	
	// Wait at most 80uS for the sensor respose
	// low-state to complete
	numMicrosDelay = 0;
	while (digitalRead(SENSOR_PIN_NUM) == LOW && numMicrosDelay < RESPONSE_TIME_US)
	{
		delayMicroseconds(1);
		++numMicrosDelay;
	}
	if (numMicrosDelay >= RESPONSE_TIME_US)
	{
		printf("ERROR READING SENSOR! OVER 80US DELAY FOR RESPONSE SIGNAL LOW\n");
		return;
	}
	#ifdef DEBUG_MODE
		numUsLowSensorResponse = numMicrosDelay;
	#endif
	
	// Wait at most 80uS for the sensor response to be high
	// before the first bit is sent.
	numMicrosDelay = 0;
	while (digitalRead(SENSOR_PIN_NUM) == HIGH && numMicrosDelay < RESPONSE_TIME_US)
	{
		delayMicroseconds(1);
		++numMicrosDelay;
	}
	if (numMicrosDelay >= RESPONSE_TIME_US)
	{
		printf("ERROR READING SENSOR! OVER 80US DELAY FOR RESPONSE SIGNAL HIGH\n");
		return;
	}
	#ifdef DEBUG_MODE
		numUsHighSensorResonse = numMicrosDelay;
	#endif	
}
