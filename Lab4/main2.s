
.section .init
.globl main
main:
	@.input
	
	ldr	r3, =0x3F200000		@ get GPIO base address
	mov	r5, r3			@ save copy for later use
	add	r5, r3, #8		@ pin 22 in register GPSEL2, so add 8 offset
	mov	r2, r3			@ move address into r2
	ldr	r2,  [r2, #0]		@ load value in register r2
	bic	r2, r2, #0b111<<6	@ clear 3 bits associated with pin 22
	str	r2, [r3, #0]		@ write value back to GPIO base
	
	@.output
	
	mov	r2, r3			@ write GPIO address to r2
	ldr	r2, [r2, #0]		@ load value at r2
	orr	r2, r2, #1<<6		@ set LSB as output for pin 22
	str	r2, [r3, #0]		@write to register r2
	
	@.set
	
	mov	r3, r5			@get base address
	add	r3, r3, #28		@ set GPSETT0 address for setting
	mov	r4, #1 			@ set bit
	mov	r2, r4		@ rotate to pin 22 and place in r2
	str	r2, [r3, #0]		@ write to memory to set
	
	
	@Pseudocode to turn off LED
	
	mov	r3, r5			@ get base address 
	add	r4, r3, #40		@ get GPCLr0 register address for setting
	mov	r4, #1			@ set bit
	mov	r2, r4		@ rotate to pin 22 and place in r2
	str	r2, [r3, #0]		@ write to memory to set
	
loop$: 
	b loop$
	

