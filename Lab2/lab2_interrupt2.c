/*
* Jim Patrizi
* Drew Borneman
* Lab 2 Group 4
*	DHT11 Polling
*/

#include <wiringPi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define DHT_PIN 4 //Data Pin to DHT

enum states {RESPONSE = 0, DATA = 1, ACQUIRED = 2, STOPPED = 3, ACQUIRING = 4, RAW_DATA_READY = 5, START_SIGNAL = 6};
volatile int state;


int dht_data[5] = {0,0,0,0,0}; 
volatile unsigned long us;
unsigned long delta;
int byteIndex = 0;
int bitIndex = 0;
int byte = 0;
int byteReadStarted = 0;

void detachInterrupt()
{
	pinMode(DHT_PIN, OUTPUT);
	pinMode(DHT_PIN, INPUT);
}


  
void dhtReadByte(int dhtPin)
{

  for (bitIndex=0 ; bitIndex < 8 ; bitIndex++) 
  {
	
		if(delta < 25)
		{
			us -= delta;
		}
		if(125 < delta && delta < 190)
		{
			state = ACQUIRING;
		}
		else {
			detachInterrupt();
			state = STOPPED;
		}

		if(60 < delta && delta < 145)
		{
			//printf("Did I get here, delta = %d\n", delta);
			if(delta > 100) //one bit came in
			{
				printf("Bit index = %d, delta = %d\n", bitIndex,delta);
				byte |= 1 << (7 - bitIndex); // Save the bit
			} // end if
		// end if
				
			if(bitIndex > 8)
				break; //we got the byte
		}
	}
  printf("my byte = %d\n", byte);	
  dht_data[byteIndex] = byte;
  byteReadStarted = 0;
}

void risingEdgeCheck(){
 	unsigned long newUs = micros();
	//printf("INTERRUPT CALLED\n");
	
	if (newUs - us > 255) 
	{
    state = STOPPED;
    detachInterrupt();
    return;
	}
	
	delta = newUs - us;
	us = newUs;
	//printf("What are my delta = %d and us = %d\n", delta, us);
	if(!byteReadStarted){
		printf("Am I here?");
		dhtReadByte(DHT_PIN);//dont want to call this everytime, need a flag to disable it after 1 trigger, and increment bitIndex after every trigger
		bitIndex = 0;
		byteReadStarted = 1;
	}
}

void start_request()
{
  state = START_SIGNAL;
  //make sure all data is 0 at the start of read for read buffer
  dht_data[0] = 0;
  dht_data[1] = 0;
  dht_data[2] = 0;
  dht_data[3] = 0;
  dht_data[4] = 0;

  //pull DHT pin down for at least 18 millsecs as per assignment (was 500us, error in assignment
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(18);
  //host pull up voltage for 40micro and wait for sensor's response
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHT_PIN, INPUT);
  us = micros();
  //printf("about to set up interrupt\n");
  if(wiringPiISR(DHT_PIN, INT_EDGE_FALLING, &risingEdgeCheck) < 0) 
  {
	printf("Unable to setup ISR\n");
	return;
  }
}

bool read_dht()
{
  state = 0;	
  FILE *fp;
  time_t ltime = time(NULL);
  
  fp = fopen("data.txt", "a");
  if(fp == NULL)
  {
	  printf("Error opening file\n");
	  exit(1);
  }
  
  start_request();

	int i;
	for (i = 0 ; i < 5 ; i++)
	{
		waitForInterrupt(DHT_PIN, 0.0001);
		byteIndex++; 
		if(byteIndex == 5)
			state = RAW_DATA_READY;
	}
  if(byteIndex == 5)
  {
	  //check all 5 bytes along with checksum. Checksum should equal lower 4 bytes binary addition result
	  if((dht_data[4] == ( (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF)))
	  {
		printf("Humidity = %d.%d and Temperature = %d.%d\n",
			  dht_data[0], dht_data[1], dht_data[2], dht_data[3]);
		fprintf(fp, "Time: %s Humidity = %d.%d and Temperature = %d.%d\n",
			  asctime(localtime(&ltime)), dht_data[0], dht_data[1], dht_data[2], dht_data[3]);
		fclose(fp); 
		return true;
	  }
	  else
	  {
		printf("Data Checksum Failure\n");
		return false;
	  }
  }
  else
  {
	  detachInterrupt();
	  delta = micros() - us;
	  
	  if(state == START_SIGNAL)
	  {
		  if(delta > 18000)
		  {
			  state = RESPONSE;
			  digitalWrite(DHT_PIN, HIGH);
			  delayMicroseconds(25);
			  pinMode(DHT_PIN, INPUT);
			  us = micros();
			  if(wiringPiISR(DHT_PIN, INT_EDGE_FALLING, &risingEdgeCheck) < 0) 
			  {
				printf("Unable to setup ISR\n");
				return 1;
			  }
			  
		  }
	  }
	  
  }
  
}


int main()
{
  if(piHiPri(99) != 0)
  {
	printf("Failed to increase priority");
	exit(EXIT_FAILURE);
  } 	
  bool failed;
  int fail_count = 0;
  //using GPIO Pins
  if(wiringPiSetupGpio() == -1)
    exit(1);
  while(1)
  {
    failed = read_dht();
    //if checksum fails, increase fail count
    if(!failed)
    {
      printf("Failure: Fail Count = %d\n", fail_count);
      fail_count++;
    }
    delay(500); //delay 1 second for temp changes
  }
  return 0;
}
