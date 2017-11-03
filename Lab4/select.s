@ Selects a function for a GPIO pin. Assumes that GPIO registers
@ are mapped to programming memory.
@ Jim Patrizi, Drew Borneman EECS 303
@ Calling sequence:
@       r0 <- address of GPIO in mapped memory
@       r1 <- pin number
@       r2 <- pin function
@       bl gpioPinFSelect

@ Define my Raspberry Pi
        .cpu    cortex-a53
        .fpu    neon-fp-armv8
        .syntax unified         @ modern syntax

@ Constants for assembler
        .equ    PIN_FIELD,0b111 @ 3 bits

@ The program
        .text
        .align  2
        .global gpioPinFSelect
        .type   gpioPinFSelect, %function
        
gpioPinFSelect:
        sub     sp, sp, 24      @ space for saving regs
                                @ (keeping 8-byte sp align)
        str     r4, [sp, 4]     @ save r4
        str     r5, [sp, 8]     @      r5
        str     r6, [sp,12]     @      r6
        str     fp, [sp, 16]    @      fp
        str     lr, [sp, 20]    @      lr
        add     fp, sp, 20      @ set our frame pointer
        
        mov     r4, r0          @ save pointer to GPIO
        mov     r5, r1          @ save pin number
        mov     r6, r2          @ save function code
        
@ Compute address of GPFSEL register and pin field        
        mov     r3, 10          @ divisor
        udiv    r0, r5, r3      @ GPFSEL number
        
        mul     r1, r0, r3      @ compute remainder
        sub     r1, r5, r1      @     for GPFSEL pin
        
@ Set up the GPIO pin funtion register in programming memory
        lsl     r0, r0, 2       @ 4 bytes in a register
        add     r0, r4, r0      @ GPFSELn address
        ldr     r2, [r0]        @ get entire register
        
        mov     r3, r1          @ need to multiply pin
        add     r1, r1, r3, lsl 1   @    position by 3
        mov     r3, PIN_FIELD   @ gpio pin field
        lsl     r3, r3, r1      @ shift to pin position
        bic     r2, r2, r3      @ clear pin field

        lsl     r6, r6, r1      @ shift function code to pin position
        orr     r2, r2, r6      @ enter function code
        str     r2, [r0]        @ update register
        
        mov     r0, 0           @ return 0;
        ldr     r4, [sp, 4]     @ restore r4
        ldr     r5, [sp, 8]     @      r5
        ldr     r6, [sp,12]     @      r6
        ldr     fp, [sp, 16]    @      fp
        ldr     lr, [sp, 20]    @      lr
        add     sp, sp, 24      @      sp
        bx      lr              @ return
