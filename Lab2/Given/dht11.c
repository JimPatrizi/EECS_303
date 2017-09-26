/********************************************************************************************
*Filename      : dht11.c
*Description   : make a thermometer
*Author        : Alan
*Website       : www.osoyoo.com
*Update        : 2017/07/06
********************************************************************************************/

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define MAXTIMINGS  85
#define DHTPIN      15  //DHT connect to TxD
int dht11_dat[5] ={0,0,0,0,0};//store DHT11 data

void read_dht11_dat()
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0,i;
    float f;//fahrenheit

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    //pull pin down to send start signal
    pinMode( DHTPIN, OUTPUT );
    digitalWrite( DHTPIN, LOW );
    delay( 18 );
    //pull pin up and wait for sensor response
    digitalWrite( DHTPIN, HIGH );
    delayMicroseconds( 40 );
    //prepare to read the pin
    pinMode( DHTPIN, INPUT );

    //detect change and read data
    for ( i = 0; i < MAXTIMINGS; i++ )
    {
        counter = 0;
        while ( digitalRead( DHTPIN ) == laststate )
        {
            counter++;
            delayMicroseconds( 1 );
            if ( counter == 255 )
            {
                break;
            }
        }
        laststate = digitalRead( DHTPIN );

        if ( counter == 255 )
            break;
        //ignore first 3 transitions
        if ( (i >= 4) && (i % 2 == 0) )
        {
            //shove each bit into the storage bytes
            dht11_dat[j / 8] <<= 1;
            if ( counter > 16 )
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }
    //check we read 40 bits(8bit x 5) +verify checksum in the last byte
    //print it out if data is good
    if ( (j >= 40) &&
         (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
    {
        f = dht11_dat[2] * 9. / 5. + 32;
        printf( "Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",
            dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f );
    }
    else
    {
        printf( "Data not good, skip\n" );
    }
}


void print_info()
{
    printf("\n");
    printf("|***************************|\n");
    printf("|           DHT11 test      |\n");
    printf("| --------------------------|\n");
    printf("| DHT11 connect to GPIO14   |\n");
    printf("| --------------------------|\n");
    printf("|                     OSOYOO|\n");
    printf("|***************************|\n");
    printf("Program is running...\n");
    printf("Press Ctrl+C to end the program\n");
}


int main( void )
{
    if ( wiringPiSetup() == -1 )
    {
        fprintf(stderr,"Can't init wiringPi: %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }
    print_info();
    while ( 1 )
    {
        read_dht11_dat();
        delay(1000);//wait ls to refresh
    }

    return(0);
}
