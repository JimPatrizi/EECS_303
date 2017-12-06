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
#include <signal.h> //for CTRL+C Stop Signal
#include <wiringPiI2C.h> //for I2C library

int main()
{
time_t ltime = time(NULL);

  while(1)
  {
    printf("Time: %s \n",asctime(localtime(&ltime)));
    printf("Temperature:");
    fflush(stdout);
    system("cat /sys/bus/iio/devices/iio\\:device0/in_temp_input");
    printf("Humidity:");
    fflush(stdout);
    system("cat /sys/bus/iio/devices/iio\\:device0/in_humidityrelative_input");
    sleep(1); //delay 1 second for temp changes
  }
  return 0;
}
