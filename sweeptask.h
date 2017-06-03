#ifndef SWEEPLTASK_H
#define SWEEPLTASK_H

#include <Arduino.h>
#include <Task.h>

class Sweeper : public TimedTask
{
public:
    // Create a new Sweeper for the specified pin and rate.
    Sweeper(uint32_t _rate);
    virtual void run(uint32_t now);
private:
    uint32_t rate;    // Blink rate.
    bool on;          // Current state of the LED.
};

Sweeper::Sweeper(uint32_t _rate)
: TimedTask(millis()),
  rate(_rate),
  on(false)
{
}

void Sweeper::run(uint32_t now)
{
    // If the LED is on, turn it off and remember the state.
    if (on) {
        Serial.println(F("Go to 0"));
        motortask.set_position(0);
        motortask.set_target_speed(800);
        motortask.start_stepping();
        on = false;
    // If the LED is off, turn it on and remember the state.
    } else {
        Serial.println(F("Go to 800"));
        motortask.set_position(800);
        motortask.set_target_speed(800);
        motortask.start_stepping();
        on = true;
    }
    // Run again in the required number of milliseconds.
    incRunTime(rate);
}



#endif
