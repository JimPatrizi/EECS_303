
#include <wiringPi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

 unsigned int currentMicroseconds= 0;


wiringPiSetup();

unsigned int getMicro()
{
	currentMicroseconds = micros();    
	return currentMicroseconds;
}

printf("Microseconds = %d", getMicro());
//use clock() in time.h