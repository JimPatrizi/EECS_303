#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <wiringPi.h>
#include <wiringPiI2C.h>

// I2C device handles.
int backLight;
int textDisplay;

// Set diaplay background color.
// Input: color RGB components. Range 0~255, inclusive.
void setBGColor(unsigned char red, unsigned char green, unsigned blue)
{
	// Initialize.
	wiringPiI2CWriteReg8(backLight, 0x00, 0x00);
	wiringPiI2CWriteReg8(backLight, 0x01, 0x00);
	
	// All LED control by PWM.
	wiringPiI2CWriteReg8(backLight, 0x08, 0xAA);
	
	// Set color component.
	wiringPiI2CWriteReg8(backLight, 0x04, red);
	wiringPiI2CWriteReg8(backLight, 0x03, green);
	wiringPiI2CWriteReg8(backLight, 0x02, blue);
}

// Internal use only.
void textCommand(unsigned char cmd)
{
    wiringPiI2CWriteReg8(textDisplay, 0x80, cmd);
}

// Set text of diaplay.
int setText(const char *string)
{
	// Clear display.
	textCommand(0x01);
	
	delay(50);
	
	// Display on, no cursor.
    textCommand(0x08 | 0x04);
    
    // Display two lines.
    textCommand(0x28);
    
    delay(50);
    
    int count = 0;
    int row = 0;
    int i = 0;
    
    for (; string[i] != '\0'; ++i) {
        if ((string[i] == '\n') || (count == 16)) {
            count = 0;
            ++row;
            if (row == 2) {
				// Reach maximum line number. Truncate any characters behind.
                break;
			}
            textCommand(0xc0);
            if (string[i] == '\n') {
                continue;
			}
		}
        ++count;
        wiringPiI2CWriteReg8(textDisplay, 0x40, string[i]);
	}
}

// Signal handler.
void sigIntHandler(int signal)
{
	// Clear screen.
	textCommand(0x01);
	
	// Disable backlight.
	setBGColor(0, 0, 0);
	
	printf("\nProgram ended with exit value -2.\n");
	exit(-2);
}

int main(int argc, const char *argv[])
{
	// Catch signal (Ctrl + C).
	struct sigaction sigHandler;
	sigHandler.sa_handler = sigIntHandler;
	sigemptyset(&sigHandler.sa_mask);
    sigHandler.sa_flags = 0;
    sigaction(SIGINT, &sigHandler, NULL);
	
	// Setup wiringPi.
	wiringPiSetup();
	
	// Setup I2C bus.
	// Backlight I2C address = 0x62
	// Text display I2C address = 0x3E
	backLight = wiringPiI2CSetup(0x62);
	textDisplay = wiringPiI2CSetup(0x3E);
	
	if((backLight == -1) || (textDisplay == -1)) {
		fprintf(stderr, "Failed to initialize I2C device. Exit.\n");
		printf("Program ended with exit value -1.\n");
		return -1;
	}
	
	// Display text.
	setText("Hello:\n      World!");
	
	int colorCount = 0;
	
	unsigned char r = (unsigned char)255;
	unsigned char g = (unsigned char)0;
	unsigned char b = (unsigned char)0;
	
	int state = -1;
	
	// Go through rainbow spectrum from red to violet.
	while (true) {
		switch (state) {
			case 0:
				// Increment Green Component.
				if (++g == 255) {
					state = 1;
				}
				break;
			case 1:
				// Decrement Red Component.
				if (--r == 0) {
					state = 2;
				}
				break;
			case 2:
				// Increment Blue Component.
				if (++b == 255) {
					state = 3;
				}
				break;
			case 3:
				// Decrement Green Component.
				if (--g == 0) {
					state = 4;
				}
				break;
			case 4:
				// Increment Red Component.
				if (++r == 255) {
					state = 5;
				}
				break;
			case 5:
				// Decrement Blue Component.
				if (--b == 0) {
					state = 0;
				}
				break;
			default:
				// Initialize.
				state = 0;
				break;
		}
		
		// Set Backlight color.
		setBGColor(r, g, b);
		
		// Delay a little bit.
		delay(10);
	}
	
	printf("Program ended with exit value 0.\n");
	return 0;
}