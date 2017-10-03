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

int dht_data[5] = {0,0,0,0,0}; 


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


//Reads each byte and accounts for delay between bytes
int dhtReadByte(int dhtPin)
{
  int i;
  int byte = 0;
  unsigned int loopCnt = 1000000;
  
  for (i=0 ; i < 8 ; i++) {
    //waits for pin to transition from high to low for data bus too come in
    while(digitalRead(dhtPin) == LOW)
    {
		if(loopCnt-- == 0)
			break;
	} // Wait for input to switch to HIGH
	delayMicroseconds(35); // Wait for digital 1 mid-point. (70micro for high, 35 is midpoint)
    //if midpoint is not high after 35 sec, we still have a 0
    if (digitalRead(dhtPin) == HIGH) //we dont care about 0s, we just skip over that bit and keep the 0
    {  //  We have a digital 1
      byte |= 1 << (7 - i); // Save the bit.
     while(digitalRead(dhtPin) == HIGH)
     {
		if(loopCnt-- == 0)
			break;
	 } 
    } // end if
  } // end for
  
  return byte;
}

//read 1 reading from dht sensor
bool read_dht()
{
	
  FILE *fp;
  time_t ltime = time(NULL);
  
  fp = fopen("data.txt", "a");
  if(fp == NULL)
  {
	  printf("Error opening file\n");
	  exit(1);
  }
  
  //start handshake
  start_request();
  pinMode(DHT_PIN, INPUT);
 
 //first look for pin to go low as per timing diagram
  unsigned int loopCnt = 1000000;
  while(digitalRead(DHT_PIN) == LOW)
  {
	if(loopCnt-- == 0) 	return -1;
  }
	loopCnt = 1000000;
 //look for pin to go high before data come in, timing between bytes/bits happens in dhtReadByte
  while(digitalRead(DHT_PIN) == HIGH)
  {
	if(loopCnt-- == 0) 	return -1;
  }
	int i;
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
  

int main()
{

//increase priority
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
