to compile and link
gcc blinker.s select.s clear.s set.s -o blinker
this blinks on pin 17
./blinker

part 2:
gcc pwm.s -o pwm
./pwm
