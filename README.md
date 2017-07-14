# TimerOne Stepper

Controls stepper motor via TimerOne.

Using PWM and interrupt handlers is much, much more efficient than accelsteppers bit-banging, our acceleration and slowdown profiles are much less optimal but do work (and they could be improved
if we wanted to).

The efficiency matters because on Arduino Fio (8Mhz) accelstepper doesn't keep up even without using softwareserial for debugging (hwserial is used for XBee).

