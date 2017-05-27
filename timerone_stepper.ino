
#include <avr/pgmspace.h>
// From http://forum.arduino.cc/index.php?topic=110307.0
#define FS(x) (__FlashStringHelper*)(x)
// My improvement for array access (idea from http://forum.arduino.cc/index.php?topic=106603.0)
#define FSA(x) FS(pgm_read_word(&(x)))
// Generic helper implementation
#define ARRAY_SIZE(a)      (sizeof(a) / sizeof(a[0]))

#define RDY_PIN 13

// Step,dir,enable
constexpr uint8_t STEPPER_PINS[] = { 9, A0, 2 };
constexpr uint8_t CFG_PINS[] = { 3, A5, A4 };

// 1/4 microstepping, stealthchop
constexpr uint8_t USTEP_FACTOR = 4;
constexpr uint8_t STEPS_PER_REV = 200;
constexpr int8_t  CFG_VALUES[] = { 1, -1, 0 }; // -1 for open, 0 for LOW, 1 for HIGH


/*********/

#define XBEE_SERIAL Serial

// Get this library from http://bleaklow.com/files/2010/Task.tar.gz (and fix WProgram.h -> Arduino.h)
// and read http://bleaklow.com/2010/07/20/a_very_simple_arduino_task_manager.html for background and instructions
#include <Task.h>
#include <TaskScheduler.h>

// Get this library from http://code.google.com/p/xbee-arduino/
#include <XBee.h>
#include "xbee_tasks.h"

#include <FastGPIO.h>
#include <TimerOne.h>
#include "motortask.h"



void apply_driver_config_pins()
{
    for (uint8_t i=0; i<3; i++)
    {
        uint8_t pin = CFG_PINS[i];
        Serial.print(F("stepstick CFG"));
        Serial.print(i+1, DEC);
        Serial.print(F(" PIN: "));
        Serial.print(pin, DEC);
        Serial.print(F(" VALUE: ")); 
        Serial.println(CFG_VALUES[i], DEC);
        switch (CFG_VALUES[i])
        {
            case 0:
                pinMode(pin, OUTPUT);
                digitalWrite(pin, LOW);
                break;
            case 1:
                pinMode(pin, OUTPUT);
                digitalWrite(pin, HIGH);
                break;
            default:
            case -1:
                pinMode(pin, INPUT);
                break;
        }
    }
}


void setup()
{
    Serial.begin(57600);
    pinMode(RDY_PIN, OUTPUT);
    pinMode(STEPPER_PINS[2], OUTPUT);
    pinMode(STEPPER_PINS[1], OUTPUT);
    pinMode(STEPPER_PINS[0], OUTPUT);
    digitalWrite(STEPPER_PINS[1], 1);
    digitalWrite(STEPPER_PINS[2], 1);

    apply_driver_config_pins();

    reset_xbee();
    // Initialize the XBee wrapper
    XBEE_SERIAL.begin(57600);
    xbee.begin(XBEE_SERIAL);
    
}


// This implements the XBee API, first byte is command identifier rest of them are command specific.
void xbee_api_callback(ZBRxResponse rx)
{
}

void loop()
{
    // Add the XBee API callback function pointer to the task
    xbeereader.callback = &xbee_api_callback;

    Task *tasks[] = { 
        &xbeereader,
        &motortask
    };
    TaskScheduler sched(tasks, NUM_TASKS(tasks));

    // Run the scheduler - never returns.
    sched.run();
}
