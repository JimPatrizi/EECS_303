
#include <wiringPi.h>
#include <softPwm.h>

//cc -o myprog myprog.c -lwiringPi -lpthread

int main()
{
	if (wiringPiSetup () == -1) //using wPi pin numbering
		exit (1) ;

	pinMode(1, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS); 
	pwmSetClock(384); //clock at 50kHz (20us tick)
	pwmSetRange(1000); //range at 1000 ticks (20ms)
	pwmWrite(1, 75);  //theretically 50 (1ms) to 100 (2ms) on my servo 30-130 works ok
	return 0 ;
}
