
#include <stdio.h>	// For printf()
#include <stdbool.h> // For booleans
#include <string.h>
#include <stdlib.h>
#include <sched.h> 
#include <time.h>	// For time functions
#include <stdint.h> // For uint8_t
#include "wiringPi.h"	// For the WiringPi library

#define TOTAL_BITS_PER_READ 40
const int MAX_PRIORITY = 99;
const int DHT_PIN = 4;
const int INIT_DELAY_MS = 20;
const int WAIT_SENSOR_US = 40;
const int RESPONSE_TIME_US = 80;
const int PRIOR_BIT_DELAY = 50;
const int TIME_FOR_ZERO_BIT_US = 28;
const int TIME_FOR_ONE_BIT_US = 70;
const int MAX_TIME = 10;
const int HUMID_INCREMENT = 0;
const int HUMID_DECREMENT = 8;
const int TEMP_INCREMENT = 16;
const int TEMP_DECREMENT = 24;
const int CHECKSUM = 32;
const int BITS_IN_BYTE = 8;
const int ERROR = -1;
const int MAX_READS = 150;
const int ONE_SECOND_US = 1000000;
const char* FILE_NAME = "eecs303group4sensorreadings.txt";


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


//Global Vars
bool readSuccess = false;

static volatile State currentState = READ_COMPLETE;
static volatile int currentReadingBitIndex = 0;
static volatile int microsPreviousRisingEdge = 0;
static volatile int bitsReceived[TOTAL_BITS_PER_READ];
static volatile bool readisReady = false;
static volatile int measuredHighOfBit[TOTAL_BITS_PER_READ];

void sensorReadISR();
char * getTimeAsString();
int generateChecksum(int * bits_rcvd);
void writeResultsToFile(int temp_int, int temp_dec, 
						int humid_int, int humid_dec,
						int checksumRead, int checksumGen,
						const char * sensorInteractionMode,
						const char * timeAsString,
						const char * errorString);




//gets sensor ready and inits state. Calls the ISR to start control first because sensor is low
void initRead()
{	
	// Set the state machine to its first state. Prepare for a read.
	currentState = INIT_PULL_LINE_LOW;
	
	// Pull the data line down for 18ms,
	// Reserve the GPIO pin
	pinMode(DHT_PIN, OUTPUT);
	digitalWrite(DHT_PIN, LOW);
	delay(INIT_DELAY_MS);
	
	// Let the ISR know that sensor is ready to read.
	readisReady = true;
	
	// Needed or else the ISR will never be executed
	// since the sensor is still low
	sensorReadISR();	
}

/*
 * Returns the 8-bit integer whose first bit is located at
 * at bits_rcvd[offset]
 */
int arrAndOffsetToInt(int * bit, int offset)
{
	int val = 0;
	int bit_idx = 0;
	for (; bit_idx < BITS_IN_BYTE; ++bit_idx)
	{
		val |= bit[offset + 7 - bit_idx] << bit_idx;
	}
	return val;
}

/*
 * Processes the data read, and writes to a file in the event that
 * the reading was successful.
 */
void analyzeAndPrintResults(int * bits, const char * errorString, const char * sensorInteractionMode)
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
	int humid_int = arrAndOffsetToInt((int *)bits, HUMID_INCREMENT);
	int humid_dec = arrAndOffsetToInt((int *)bits, HUMID_DECREMENT);
	int temp_int = arrAndOffsetToInt((int *)bits, TEMP_INCREMENT);
	int temp_dec = arrAndOffsetToInt((int *)bits, TEMP_DECREMENT);
	int checksum_read = arrAndOffsetToInt((int *)bits, CHECKSUM);
	int checksum_generated = generateChecksum((int *)bits);
	
	if (checksum_read == checksum_generated && humid_int > 0 && temp_int > 0)
	{
		readSuccess = true;
	}
	
	// Print the results
	printf("Temp: %d.%d\n", temp_int, temp_dec);
	printf("Humidity: %d.%d\n", humid_int, humid_dec);
	
	// Write the results to a file
	writeResultsToFile(temp_int, temp_dec, humid_int, humid_dec, checksum_read, checksum_generated, sensorInteractionMode, timeAsString, errorString);
}

void writeResultsToFile(int temp_int, int temp_dec, 
						int humid_int, int humid_dec,
						int checksumRead, int checksumGen,
						const char * sensorInteractionMode,
						const char * timeAsString,
						const char * errorString)
{
	// Open the file
	FILE * outFp = fopen(FILE_NAME, "a");
	
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
	
	if(fclose(outFp) != 0)
	{
		printf("\nERROR CLOSING FILE!\n");
	}
}

/*
 * Initializes the GPIO pin to high
 */
void setupGPIO()
{
	  //using GPIO Pins
	if(wiringPiSetupGpio() == -1){
		exit(1);
	}
	
	// Reserve the GPIO pin
	pinMode(DHT_PIN, OUTPUT);
	
	// Set the line high by default
	digitalWrite(DHT_PIN, HIGH);
	
	//from wiringPi
	pullUpDnControl(DHT_PIN, PUD_UP);
}

// Releases the GPIO reservation.
void releaseGpio()
{
	system("gpio-admin export 4");
	system("gpio-admin unexport 4");
}

/*
 * Generates a checksum from the humidity and temp readings.
 */
int generateChecksum(int * bits_rcvd)
{
	// Use uint8_t variables to ensure that the result of each addition
	// is only eight bits.
	uint8_t humid_int = arrAndOffsetToInt(bits_rcvd, HUMID_INCREMENT);
	uint8_t humid_dec = arrAndOffsetToInt(bits_rcvd, HUMID_DECREMENT);
	uint8_t temp_int = arrAndOffsetToInt(bits_rcvd, TEMP_INCREMENT);
	uint8_t temp_dec = arrAndOffsetToInt(bits_rcvd, TEMP_DECREMENT);
	
	uint8_t checksum = humid_int + humid_dec + temp_int + temp_dec;
	return checksum;
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
 * ISR for reading the sensor. There are several states which this ISR accounts for.
 */
void sensorReadISR()
{
	switch (currentState)
	{
		case INIT_PULL_LINE_LOW :
			if (readisReady == true)
			{
				currentReadingBitIndex = 0;
				
				microsPreviousRisingEdge = 0;
				
				currentState = INPUT_JUST_ENABLED;
				
				digitalWrite(DHT_PIN, HIGH);
				
				// Change the pin the input mode.
				pinMode(DHT_PIN, INPUT);
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
			; 
			
			// Get the time when this edge occurred
			int microsCurrentEdge = micros();
			
			// Get the amount of uS the bit-determining pulse was high.
			// Subtract 50us, the pre-bit delay.
			int prevBitHighTime = microsCurrentEdge - microsPreviousRisingEdge - PRIOR_BIT_DELAY;
			
			microsPreviousRisingEdge = microsCurrentEdge;
			
			// If this is the first bit, we have no reference for a differential reading.
			// The read time has been set, so break and wait for the next rising edge
			// to determine how long this cycle was high.
			if (currentReadingBitIndex == 0)
			{
				++currentReadingBitIndex;
				break;
			}
			
			// This section determines the whether the last bit read was a 1, 0, or error.
			if (prevBitHighTime <= TIME_FOR_ZERO_BIT_US + MAX_TIME)
			{
				bitsReceived[currentReadingBitIndex - 1] = 0;
			}
			// Account for if the line is high for longer than the amount of
			// time that is typically required for a 1. This indicates an error.
			else if (prevBitHighTime > TIME_FOR_ONE_BIT_US + MAX_TIME)
			{
				bitsReceived[currentReadingBitIndex] = ERROR;
				currentState = ERROR_STATE;
			}
			else
			{
				bitsReceived[currentReadingBitIndex - 1] = 1;
			}
			
			// Account for the this bit's high time.
			measuredHighOfBit[currentReadingBitIndex - 1] = prevBitHighTime;	
				
			++currentReadingBitIndex;
			
			// The -1 accounts for a 0-initial value for the idx
			// At this point, the last bit time has to be read, so delay
			// 40, then poll the input to see if it's high.
			if (currentReadingBitIndex >= TOTAL_BITS_PER_READ)
			{
				currentState = READ_COMPLETE;
				
				//delayed needed here to work
				delayMicroseconds(28);
				
				if (digitalRead(DHT_PIN) == LOW)
				{
					bitsReceived[currentReadingBitIndex - 1] = 0;
				}
				else
				{
					bitsReceived[currentReadingBitIndex - 1] = 1;
				}
			}
			
			break;
			
		default :
			break;
	}
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

int main()
{
	if(piHiPri(MAX_PRIORITY) == ERROR)
	{
	printf("Failed to increase priority");
	exit(EXIT_FAILURE);
	}

	//sets GPIO to OUTPUT HIGH
	setupGPIO(); 	
  
	// Register an interupt for a rising edge.
	if (wiringPiISR(DHT_PIN, INT_EDGE_RISING, sensorReadISR) < 0)
	{
		printf("ERROR SETTING ISR!\n");
		currentState = ERROR_STATE;
	}
	
	// Attempt MAX_NUM_READS measurements in 1-second intervals.
	// Stop if a successful measurement has been taken.
	int readIdx = 0;
	while (!readSuccess && readIdx < MAX_READS)
	{
		//Clear bits Bus
		memset((int *)bitsReceived, ERROR, TOTAL_BITS_PER_READ * sizeof(int));
		memset((int *)measuredHighOfBit, ERROR, TOTAL_BITS_PER_READ * sizeof(int));
		
		//reset control
		readisReady = false;
		
		//Pull line low for 18ms and resets SM
		initRead();
		
		//delay for ISR
		delayMicroseconds(ONE_SECOND_US);
		
		if(currentState == ERROR_STATE)
		{
			analyzeAndPrintResults((int *) bitsReceived, "Error reading sensor\n", "interrupts");
		}
		else
		{
			analyzeAndPrintResults((int *) bitsReceived, NULL, "interrupts");
		}
		
		++readIdx;
	}
	releaseGpio();
	
	return 0;	
}


