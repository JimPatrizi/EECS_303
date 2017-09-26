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

#define DHT_PIN 4 //Data Pin to DHT, changed to non broadcom

int dht_data[5] = {0,0,0,0,0}; 
int currentPosition = 0;
int currentBit = 0;

void risingEdgeCheck(){
	
	printf("INTERRUPT CALLED\n");
	delayMicroseconds(40);
	if (digitalRead(DHT_PIN) == HIGH){
		currentBit = 1;
	}else if (digitalRead(DHT_PIN) == LOW){
		currentBit = 0;
	}
	currentPosition++;
	printf("%d, %d\n", currentBit, currentPosition);
}


void start_request()
{
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
}


int dhtReadByte(int dhtPin)
{
  int i;
  int byte = 0;
  
  	printf("about to set up interrupt");
	if(wiringPiISR(DHT_PIN, INT_EDGE_RISING, &risingEdgeCheck) < 0) {
	printf("Unable to setup ISR\n");
	return 1;
	}
	printf("Finish setting up interrupt");
  
  while(currentPosition < 8){
	  	//printf("putting a %d at %d for a byte total of %d\n",currentBit, currentPosition, byte);
      byte |= currentBit << (7 - currentPosition);
    }
  currentPosition = 0;
  	//printf("%d\n", byte);
  return byte;
}



bool read_dht()
{
	
  FILE *fp;
  time_t ltime = time(NULL);
  
  fp = fopen("/home/group4/Lab2/data/data.txt", "a");
  if(fp == NULL)
  {
	  printf("Error opening file\n");
	  exit(1);
  }
  
  start_request();
  pinMode(DHT_PIN, INPUT);
  //system(gpio edge 4 rising);
 
 
  unsigned int loopCnt = 1000000;
  while(digitalRead(DHT_PIN) == LOW)
  {
	if(loopCnt-- == 0) 	return -1;
  }
	loopCnt = 1000000;
  while(digitalRead(DHT_PIN) == HIGH)
  {
	if(loopCnt-- == 0) 	return -1;
  }
	int i;
	system("gpio-admin export 4");
	    system("echo out > /sys/class/gpio/gpio4/direction");
	system("gpio-admin unexport 4");
	
	printf("about to set up interrupt");
	if(wiringPiISR(DHT_PIN, INT_EDGE_RISING, &risingEdgeCheck) < 0) {
	printf("Unable to setup ISR\n");
	return 1;
	}
			
	//printf("About to read bytes of data\n");
	for (i = 0 ; i < 5 ; i++)
	{
		dht_data[i] = dhtReadByte(DHT_PIN); // Read five bytes from DHT
	}
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
  // Do all the receiving job here...
// Check if checksum is correct, if yes return true, if not
//return false.
//booldata type is declared in header file  stdbool.h

//And when you try to get a data, you may use the following code:
//while(readDHTSensor() == false);


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
    delay(1000); //delay 1 second for temp changes
  }
  return 0;
}
