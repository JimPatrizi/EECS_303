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

enum states {RESPONSE = 0, DATA = 1, 
			  ACQUIRED = 2, STOPPED = 3, ACQUIRING = 4,
			  RAW_DATA_READY = 5, START_SIGNAL_DONE = 6};
			  
enum states state;

//5 byte reading bus
int dht_data[5] = {0,0,0,0,0}; 
unsigned int currentMicroseconds = 0;
unsigned int prevMicroseconds = 0;
unsigned int delta = 0;
int response_signal = 0;
int bitIndex = 0;

//int byteIndex = 0;
//int bitIndex = 0;
//int byte = 0;
//int byteReadStarted = 0;

//ISR routine
void ISR_fallCheck(){
 	unsigned int pastMicroseconds= micros();
	//if (newUs - us > 255) 
	//{
    //state = STOPPED;
    //detachInterrupt();
    //return;
	//}
	delta = currentMicroseconds - prevMicroseconds;
	//printf("Current Microseconds = %lf\n Prev Microseconds = %lf\n", (double) currentMicroseconds, (double) prevMicroseconds);
	prevMicroseconds = currentMicroseconds;
	if(delta > 95 && response_signal)
		state = DATA; //1 came in
	else if((delta < 95 && delta > 70) && response_signal)//0 came in
		state = ACQUIRING;
}

void start_request()
{
  state = START_SIGNAL_DONE;
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

  //init interrupt
  if(wiringPiISR(DHT_PIN, INT_EDGE_FALLING, &ISR_fallCheck) < 0) 
  {
	printf("Unable to setup ISR\n");
	return;
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
  
  state = STOPPED;  
  int byte = 0;
  int byteIndex = 0;
  FILE *fp;
  time_t ltime = time(NULL);

  while(1)
  {
	switch (state)
	{
		case START_SIGNAL_DONE :
			printf("did I get to start_signal done?\n");
			state = RESPONSE;
		break;
		
		case RESPONSE	:
			response_signal = 1;
			//waitForInterrupt(DHT_PIN, -1);
		break;
		
		case ACQUIRING	:
			printf("did I get to acquiring?\n");
			if(bitIndex == 7)
			{
				state = ACQUIRED;
				bitIndex = 0;
			}
			
			else
			{	
				bitIndex++;//acquired a 0, moving on
				//waitForInterrupt(DHT_PIN, -1);
			}
		break;
		
		case DATA	:
			printf("did I get to data?\n");
			byte |= 1 << (7 - bitIndex); // Save the 1 bit.
			if(bitIndex == 7){
				state = ACQUIRED;
				bitIndex = 0;
			}
			else {
				bitIndex++;
				//waitForInterrupt(DHT_PIN, -1);
			}
		break;
		
		case ACQUIRED	:
			printf("did I get to acquired?\n");
			dht_data[byteIndex] = byte;
			byte = 0;
			byteIndex++;
		break;
		
		case RAW_DATA_READY		:
			printf("did I get to raw?\n");
			byteIndex = 0;
			bitIndex = 0;
			//check all 5 bytes along with checksum. Checksum should equal lower 4 bytes binary addition result
			if((dht_data[4] == ( (dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) & 0xFF)))
			{
			fp = fopen("data.txt", "a");
			if(fp == NULL)
			{
			printf("Error opening file\n");
			exit(1);
			}
			printf("Humidity = %d.%d and Temperature = %d.%d\n",
				dht_data[0], dht_data[1], dht_data[2], dht_data[3]);
				fprintf(fp, "Time: %s Humidity = %d.%d and Temperature = %d.%d\n",
				asctime(localtime(&ltime)), dht_data[0], dht_data[1], dht_data[2], dht_data[3]);
				fclose(fp);      
			}
			else
			 {
				printf("Data Checksum Failure\n");
			 }
			 
			delay(1000); //delay 1 second for temp changes
			state = STOPPED;
						
		break;
		
		default	:
		{
			printf("did I get to default?\n");
			start_request();
		}	
	}
  }
  return 0;
}
