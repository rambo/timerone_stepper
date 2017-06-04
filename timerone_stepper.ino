
#include <avr/pgmspace.h>
// From http://forum.arduino.cc/index.php?topic=110307.0
#define FS(x) (__FlashStringHelper*)(x)
// My improvement for array access (idea from http://forum.arduino.cc/index.php?topic=106603.0)
#define FSA(x) FS(pgm_read_word(&(x)))
// Generic helper implementation
#define ARRAY_SIZE(a)      (sizeof(a) / sizeof(a[0]))
//#define MOTOR_REPORT_TO 0x0013a200, 0x403276df
#define MOTOR_REPORT_TO 0x0, 0x0


#define USE_XBEE
// Get this library from http://code.google.com/p/xbee-arduino/
#include <SoftwareSerial.h>
SoftwareSerial swSerial(A0, A1); // rx,tx
#define DEBUG_SERIAL swSerial
//#define DEBUG_SERIAL Serial
//#define VERBOSE_DEBUG

#define RDY_PIN 13

// Step,dir,enable
constexpr uint8_t STEPPER_PINS[] = { 9, 10, 3 };
constexpr uint8_t CFG_PINS[] = { 4, 5, 6 };
constexpr uint8_t HOMESW_PIN = 11;

// 1/4 microstepping, stealthchop
constexpr uint8_t USTEP_FACTOR = 4;
constexpr uint8_t STEPS_PER_REV = 200;
constexpr uint8_t MAX_RPS = 2;
constexpr int8_t  CFG_VALUES[] = { 1, -1, 0 }; // -1 for open, 0 for LOW, 1 for HIGH


/*********/


// Get this library from http://bleaklow.com/files/2010/Task.tar.gz (and fix WProgram.h -> Arduino.h)
// and read http://bleaklow.com/2010/07/20/a_very_simple_arduino_task_manager.html for background and instructions
#include <Task.h>
#include <TaskScheduler.h>
#include "valueparser.h"

#include <Bounce2.h>
#include <FastGPIO.h>
#include <TimerOne.h>
#include "motortask.h"


#ifdef USE_XBEE
#define XBEE_SERIAL Serial
#include <XBee.h>
#include "xbee_tasks.h"
#else
//#include "serialtask.h"
#include "sweeptask.h"
#endif


XBeeAddress64 coordinator_addr64 = XBeeAddress64(MOTOR_REPORT_TO);
uint8_t motor_payload[] = {
  'M', // Identify as motor packet
  0x0, // type
  0x0,0x0,0x0,0x0, // int32_t, current_position
  0x0,0x0,0x0,0x0, // int32_t, target_position
  0x0 // homing
};
ZBTxRequest zb_motor_Tx = ZBTxRequest(coordinator_addr64, motor_payload, sizeof(motor_payload));

void pack_payload_position(uint8_t idx, int32_t pos)
{
    motor_payload[idx] = pos >> 24;
    motor_payload[idx+1] = (0x00ffffff & pos) >> 16;
    motor_payload[idx+2] = (0x0000ffff & pos) >> 8;
    motor_payload[idx+3] = (0x000000ff & pos);
}

void debug_print_payload_position(uint8_t idx, uint8_t num_bytes)
{
#ifdef DEBUG_SERIAL
    uint8_t tgt = idx + num_bytes;
    for (uint8_t i=idx; i < tgt ; i++)
    {
        DEBUG_SERIAL.print(F("motor_payload["));
        DEBUG_SERIAL.print(i, DEC);
        DEBUG_SERIAL.print(F("]=0x"));
        DEBUG_SERIAL.println(motor_payload[i], HEX);
      
    }
#endif
}


#include "reporttask.h"



void apply_driver_config_pins()
{
    for (uint8_t i=0; i<3; i++)
    {
        uint8_t pin = CFG_PINS[i];
#if defined(DEBUG_SERIAL) && defined(VERBOSE_DEBUG)
        DEBUG_SERIAL.print(F("stepstick CFG"));
        DEBUG_SERIAL.print(i+1, DEC);
        DEBUG_SERIAL.print(F(" PIN: "));
        DEBUG_SERIAL.print(pin, DEC);
        DEBUG_SERIAL.print(F(" VALUE: ")); 
        DEBUG_SERIAL.println(CFG_VALUES[i], DEC);
#endif
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
    digitalWrite(STEPPER_PINS[1], 1);
    digitalWrite(STEPPER_PINS[2], 1);

    
    // Extra GND pins    
    pinMode(12, OUTPUT);
    digitalWrite(12, 0);
    pinMode(A2, OUTPUT);
    digitalWrite(A2, 0);

    apply_driver_config_pins();

#ifdef USE_XBEE
    reset_xbee();
    // Initialize the XBee wrapper
    XBEE_SERIAL.begin(57600);
    xbee.begin(XBEE_SERIAL);
#endif
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.begin(57600);
    DEBUG_SERIAL.println(F("Booted"));
#endif
}


#ifdef USE_XBEE
// This implements the XBee API, first byte is command identifier rest of them are command specific.
void xbee_api_callback(ZBRxResponse rx)
{
    switch(rx.getData(0))
    {
        case 'h':
        case 'H':
        {
#ifdef DEBUG_SERIAL
            DEBUG_SERIAL.println(F("Going home"));
#endif
            motortask.go_home();
            break;
        }
        case 's':
        case 'S':
        {
#ifdef DEBUG_SERIAL
            DEBUG_SERIAL.println(F("Stopping"));
#endif
            motortask.hard_stop();
            break;
        }
        case 'f':
        case 'F':
        {
            uint16_t maxspeed = ardubus_hex2uint(rx.getData(1),rx.getData(2),rx.getData(3), rx.getData(4));
#ifdef DEBUG_SERIAL
            DEBUG_SERIAL.print(F("Setting max_speed"));
            DEBUG_SERIAL.println(maxspeed, DEC);
#endif
            motortask.set_max_speed(maxspeed);
            break;
        }
        case 'g':
        case 'G':
        {
            int32_t go_to = ardubus_hex2long(rx.getData(1),rx.getData(2),rx.getData(3), rx.getData(4),rx.getData(5),rx.getData(6),rx.getData(7), rx.getData(8));
#ifdef DEBUG_SERIAL
            DEBUG_SERIAL.print(F("Going to "));
            DEBUG_SERIAL.println(go_to, DEC);
#endif
            motortask.set_position(go_to);
            motortask.set_target_speed(motortask.max_speed);
            motortask.start_stepping();
            break;
        }
    }
}


void motor_stop_callback()
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("motor_stop_callback called"));
#endif
    motor_payload[1] = 'S'; // Stop report
    pack_payload_position(2, motortask.current_position);
    pack_payload_position(6, motortask.target_position);
    motor_payload[10] = motortask.homing;
    debug_print_payload_position(0, sizeof(motor_payload));
    xbee.send(zb_motor_Tx);
}
#endif

void loop()
{
    Reporter reporttask(5000);

#ifdef USE_XBEE
    // Add the XBee API callback function pointer to the task
    motortask.stop_callback = &motor_stop_callback;
    xbeereader.callback = &xbee_api_callback;
    Task *tasks[] = { 
        &xbeereader,
        &motortask,
        &reporttask
    };
#else
    Sweeper sweeptask(10000);
    Task *tasks[] = { 
        &sweeptask,
        &motortask,
        &reporttask
    };
#endif
    motortask.go_home();
    DEBUG_SERIAL.println(F("Starting tasks"));
    TaskScheduler sched(tasks, NUM_TASKS(tasks));

    // Run the scheduler - never returns.
    sched.run();
}
