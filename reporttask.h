#ifndef REPORTTASK_H
#define REPORTTASK_H

#include <Arduino.h>
#include <Task.h>

class Reporter : public TimedTask
{
public:
    // Create a new Reporter for the specified pin and rate.
    Reporter(uint32_t _rate);
    virtual void run(uint32_t now);
private:
    uint32_t rate;    // Blink rate.
};

Reporter::Reporter(uint32_t _rate)
: TimedTask(millis()),
  rate(_rate)
{
}

void Reporter::run(uint32_t now)
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.print(F("Report! now="));
    DEBUG_SERIAL.println(now, DEC);
    DEBUG_SERIAL.print(F("motortask.current_position="));
    DEBUG_SERIAL.println(motortask.current_position, DEC);
    DEBUG_SERIAL.print(F("motortask.target_position="));
    DEBUG_SERIAL.println(motortask.target_position, DEC);
    DEBUG_SERIAL.print(F("motortask.stepdir="));
    DEBUG_SERIAL.println(motortask.stepdir, DEC);
    DEBUG_SERIAL.print(F("motortask.homing="));
    DEBUG_SERIAL.println(motortask.homing, DEC);
#endif
    // Run again in the required number of milliseconds.
    incRunTime(rate);
}



#endif
