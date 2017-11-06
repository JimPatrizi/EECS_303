@ ---------------------------------------
@       Data Section
@ ---------------------------------------
        
         .data
         .balign 4      
Intro:   .asciz  "Raspberry Pi wiringPi pwm test\n"
ErrMsg:  .asciz "Setup didn't work... Aborting...\n"
LedPin:  .int   1
i:       .int   1024
delayMs: .int   10
PWM_OUTPUT   =     2
        
@ ---------------------------------------
@       Code Section
@ ---------------------------------------
        
        .text
        .global main
        .extern printf
        .extern wiringPiSetup
        .extern delay
        .extern digitalWrite
        .extern analogWrite
	    .extern pwmWrite
        .extern pinMode
        .extern pwmSetMode
        
main:   push    {ip, lr}        @ push return address + dummy register
                                @ for alignment

@  printf( "pwm..." ) ;
        ldr     r0, =Intro      
        bl      printf          

@  if (wiringPiSetup() == -1) {
@     printf( "Setup didn't work... Aborting." ) ;
@     exit (1)                                   ;
@  }
        bl      wiringPiSetup
        mov     r1,#-1
        cmp     r0, r1
        bne     init
        ldr     r0, =ErrMsg
        bl      printf
        b       done

@  pinMode(pin, OUTPUT)         ;
init:
        mov	r0, #1
    	mov	r1, #2
    	bl	pinMode    

@   for ( i=1024; i>0; i-- ) {
        ldr     r4, =i			@set r4 to the address of i
        ldr     r4, [r4]		@then load the contents (#1024) because ARM doesn't handle more than 8 bit numbers as a direct operand
        mov     r5, #1			
forLoop:
        cmp     r4, r5			@if counter at r4 is less than 1, exit the loop
        blt     done
        
@       pwmWrite(pin, 1) ;
        ldr     r0, =LedPin		
        ldr     r0, [r0]
        mov     r1, r4
        bl      pwmWrite

@       delay(1)               ;
        ldr     r0, =delayMs
        ldr     r0, [r0]
        bl      delay

        sub     r4, #1		@decrement the counter
        b       forLoop
done:   
        pop     {ip, pc}        @ pop return address into pc
