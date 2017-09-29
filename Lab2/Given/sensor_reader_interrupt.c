// This file contains the interrupt-specific functions.
// Due to timing inconsisties, the 20-40us period after a read is initiated
// is entirely ignored.
// To compile: gcc sensor_reader_common.c sensor_reader_interrupt.c -o sensor_reader_interrupt -lwiringPi -Wall

#include <stdio.h>	// For printf()
#include <stdbool.h> // For booleans
#include <string.h>	// For memset()
#include <stdlib.h>

#include "wiringPi.h"	// For the WiringPi library
#include "sensor_reader.h" // Function declarations and constant definitions.

// Globals
static volatile State currentState = READ_COMPLETE;
static volatile int currentReadingBitIdx = 0;
static volatile int microsPreviousRisingEdge = 0;
static volatile int bitsRcvd[TOTAL_BITS_PER_READ];
static volatile bool readReady = false;
static volatile int measuredBitHighTime[TOTAL_BITS_PER_READ];

int main()
{
	// Set this process to have maximum priority.
	if (piHiPri(MAX_PRIORITY) == ERROR)
	{
		printf("Error setting priority! Exiting");
		return ERROR;
	}
		
	// Prep the GPIO pin
	setupGpio();
	
	
	// Register an interupt for a rising edge.
	// This function alone takes on the order of 20ms to execute.
	if (wiringPiISR(SENSOR_PIN_NUM, INT_EDGE_RISING, sensorReadISR) < 0)
	{
		printf("ERROR SETTING ISR!\n");
		currentState = ERROR_STATE;
	}		
		
	// Attempt MAX_NUM_READS measurements in 1-second intervals.
	// Stop if a successful measurement has been taken.
	int readIdx = 0;
	while (!readSuccessful && readIdx < MAX_NUM_READS)
	{
		// Print the current sample number
		printf("\nSample Number: %i\n", readIdx);
		
		// Reset the bitsRcvd array
		memset((int *)bitsRcvd, ERROR, TOTAL_BITS_PER_READ * sizeof(int));
		memset((int *)measuredBitHighTime, ERROR, TOTAL_BITS_PER_READ * sizeof(int));
	
		// Reset the readReady control variable.
		readReady = false;
	
		// Pull the line low for 20ms and reset the state machine
		initiateRead();
		
		// Do nothing for one second while the sensor is read.
		// The ISR will be able to run during this period.
		delayMicroseconds(ONE_SEC_IN_US);
		
		// Write results to console and output.
		if (currentState == ERROR_STATE)
		{
			// Read should be done. Analyze the results.
			analyzeAndPrintResults((int *)bitsRcvd, "Error reading from the sensor\n", "interrupts");
		}
		else
		{
			analyzeAndPrintResults((int *)bitsRcvd, NULL, "interrupts");
		}
		
		++readIdx;
	}
		
	// Free the GPIO
	releaseGpio();
	
	return 0;
}

/*
 * Prepares the sensor for reading. This function sets the state machine
 * to its initial state, pulls (and keeps low) the sensor pin,
 * and sets readReady to true. It then calls the ISR directly
 * in order to let the sensor take over controlling the line.
 */
void initiateRead()
{	
	// Set the state machine to its first state. Prepare for a read.
	currentState = INIT_PULL_LINE_LOW;
	
	// Pull the data line down for 20ms,
	// then pull it back up. The datasheet
	// says 500us, but this doesn't work.
	// Reserve the GPIO pin
	pinMode(SENSOR_PIN_NUM, OUTPUT);
	digitalWrite(SENSOR_PIN_NUM, LOW);
	delay(READ_INIT_DELAY_MS);
	
	// Let the ISR know that sensor is ready to read.
	readReady = true;
	
	// Needed or else the ISR will never be executed
	// (since the sensor is still low and needs to be pulled up).
	sensorReadISR();	
}

/*
 * ISR for reading the sensor. There are several states which this ISR accounts for.
 */
void sensorReadISR()
{
	switch (currentState)
	{
		case INIT_PULL_LINE_LOW :
			if (readReady == true)
			{
				currentReadingBitIdx = 0;
				
				microsPreviousRisingEdge = 0;
				
				currentState = INPUT_JUST_ENABLED;
				
				digitalWrite(SENSOR_PIN_NUM, HIGH);
				
				// Change the pin the input mode.
				pinMode(SENSOR_PIN_NUM, INPUT);
			}
			break;
		
		// Temporary, single-use state. It serves to handle
		// the case where the sensor pin is set to an input
		// in the INIT_PULL_LINE_LOW state, triggering an
		// unwanted interrupt.
		case INPUT_JUST_ENABLED:
			currentState = HIGH_ACK;
			break;
			
		// Entered at the beginning of the high response
		case HIGH_ACK :
			currentState = BIT_READ_RISING;
			break;

		case BIT_READ_RISING:
			; // Yes, this is intentional. It allows the variable to be declared immediately following a label.
			
			// Get the time when this edge occurred
			int microsCurrentEdge = micros();
			
			// Get the amount of uS the bit-determining pulse was high.
			// Subtract 50us, the pre-bit delay.
			int prevBitHighTime = microsCurrentEdge - microsPreviousRisingEdge - PRE_BIT_DELAY_US;
			
			microsPreviousRisingEdge = microsCurrentEdge;
			
			// If this is the first bit, we have no reference for a differential reading.
			// The read time has been set, so break and wait for the next rising edge
			// to determine how long this cycle was high.
			if (currentReadingBitIdx == 0)
			{
				++currentReadingBitIdx;
				break;
			}
			
			// This section determines the whether the last bit read was a 1, 0, or error.
			if (prevBitHighTime <= MAX_TIME_FOR_ZERO_BIT_US + MAX_TIME_BUFFER)
			{
				bitsRcvd[currentReadingBitIdx - 1] = 0;
			}
			// Account for if the line is high for longer than the amount of
			// time that is typically required for a 1. This indicates an error.
			else if (prevBitHighTime > MAX_TIME_FOR_ONE_BIT_US + MAX_TIME_BUFFER)
			{
				bitsRcvd[currentReadingBitIdx] = ERROR;
				currentState = ERROR_STATE;
			}
			else
			{
				bitsRcvd[currentReadingBitIdx - 1] = 1;
			}
			
			// Account for the this bit's high time.
			measuredBitHighTime[currentReadingBitIdx - 1] = prevBitHighTime;	
				
			++currentReadingBitIdx;
			
			// The "-1" accounts for a 0-initial value for the idx
			// At this point, the last bit time has to be read, so delay
			// 40, then poll the input to see if it's high.
			// If it is, then the current bit must be a 1.
			// The side effect to this is that if this bit is 
			// held too long, we won't be able to tell.
			if (currentReadingBitIdx >= TOTAL_BITS_PER_READ)
			{
				currentState = READ_COMPLETE;
				
				// Frankly, this choice is arbitrary.
				delayMicroseconds(28);
				
				if (digitalRead(SENSOR_PIN_NUM) == LOW)
				{
					bitsRcvd[currentReadingBitIdx - 1] = 0;
				}
				else
				{
					bitsRcvd[currentReadingBitIdx - 1] = 1;
				}
			}
			
			break;
			
		default :
			break;
	}
}
