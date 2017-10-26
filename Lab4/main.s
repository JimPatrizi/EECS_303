
.section .init
.globl main
main:

	@Setting up GPIO pin 18 as output
	ldr r0,=0x3F200000
	mov r1,#1
	lsl r1,#24			@Each GPIO pin has 3 bits, using GPIO pin 18 so 24 bit offset
	str r1,[r0,#4]		@Offset of 4 for the GPIO pin 10-19 group
	
	@turn on the LED
	mov r1,#1
	lsl r1,#18
	str r1,[r0,#40]
	
loop$: 
	b loop$
