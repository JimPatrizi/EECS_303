@ Jim Patrizi EECS 303

@ Define my Raspberry Pi
        .cpu    cortex-a53
        .fpu    neon-fp-armv8
        .syntax unified
        
@ The program
        .text
        .align  2
        .global main
        .type   main, %function
        
main:

	.input
	
	LDR	R3, .gpiobase		// get GPIO base address
	MOV	R5, R3			// save copy for later use
	ADD	R5, R3, #8		// pin 22 in register GPSEL2, so add 8 offset
	MOV	R2, R3			// move address into R2
	LDR	R2  [R2, #0]		// load value in register R2
	BIC	R2, R2, #0b111<<6	// clear 3 bits associated with pin 22
	STR	R2, [R3, #0]		// write value back to GPIO base
	
	.output
	
	MOV	R2, R3			// write GPIO address to R2
	LDR	R2, [R2, #0]		// load value at R2
	ORR	R2, R2, #1<<6		// set LSB as output for pin 22
	STR	R2, [R3, #0]		//write to register R2
	
	.set
	
	MOV	R3, R5			//get base address
	ADD	R3, R3, #28		// set GPSETT0 address for setting
	MOV	R4, #1 			// set bit
	MOV	R2, R4, LSB #22		// rotate to pin 22 and place in R2
	STR	R2, [R3, #0]		// write to memory to set
	
	
	/* Pseudocode to turn off LED  */
	
	MOV	R3, R5			// get base address 
	ADD	R4, R3, #40		// get GPCLR0 register address for setting
	MOV	R4, #1			// set bit
	MOV	R2, R4, LSB #22		// rotate to pin 22 and place in R2
	STR	R2, [R3, #0]		// write to memory to set
	

