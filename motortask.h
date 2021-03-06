#ifndef MOTORTASKS_H
#define MOTORTASKS_H

#include <Arduino.h>
#include <Task.h>
#include <FastGPIO.h>
#include <TimerOne.h>
#include <Bounce2.h>


#define SPEED_ADJUST_INTERVAL 50
#define MIN_PPS 20
#define PPS_SNAP_LIMIT 80
constexpr uint16_t MAX_PPS = USTEP_FACTOR * STEPS_PER_REV * MAX_RPS;
constexpr uint8_t DTG_SPEED_FACTOR = 2;


typedef void (*MotoTaskvoidFuncPtr)(void);

void countstep(void);

class MotorTask : public Task
{
public:
    MotorTask();
    virtual void run(uint32_t now);
    virtual bool canRun(uint32_t now);
    void start_stepping();
    void hard_stop();
    void go_home();
    void home_done();
    void set_pps(uint16_t pps);
    void accelerate();
    void set_target_speed(uint16_t speed);
    void set_max_speed(uint16_t speed);
    void set_position(int32_t tgt_position);
    volatile bool stepped;
    volatile int32_t current_position;
    bool homing;
    int8_t stepdir;
    int32_t target_position;
    uint32_t last_speed_adjust;
    uint16_t target_speed = MAX_PPS;
    uint16_t max_speed = MAX_PPS;
    uint16_t current_speed;
    Bounce homesw = Bounce();
    MotoTaskvoidFuncPtr stop_callback;

};

MotorTask::MotorTask()
: Task()
{
    pinMode(RDY_PIN, OUTPUT);
    pinMode(HOMESW_PIN, INPUT_PULLUP);
    homesw.attach(HOMESW_PIN);
    homesw.interval(5); // interval in ms
}


// We can't just return true since then no other task could ever run (since we have the priority)
bool MotorTask::canRun(uint32_t now)
{
    homesw.update();
    if (homing)
    {
        if (!homesw.read())
        {
            this->home_done();
        }
    }
    if (stepped)
    {
        stepped = false;
        return true;
    }
    if (target_speed != current_speed)
    {
        if ((now - last_speed_adjust) > SPEED_ADJUST_INTERVAL)
        {
            return true;
        }
    }
    return false;
}

void MotorTask::set_position(int32_t tgt_position)
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("set_position called"));
#endif
    // TODO: Check that we do not attempt to change direction instantly
    target_position = tgt_position;
    if (target_position < current_position)
    {
        stepdir = -1;
    }
    else
    {
        stepdir = 1;
    }
}

void MotorTask::go_home(void)
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("go_home called"));
#endif
    target_position = 0;
    homing = true;
    stepdir = -1;
    this->set_max_speed(MAX_PPS);
    this->set_target_speed(MAX_PPS);
    this->start_stepping();
}

void MotorTask::home_done(void)
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("home_done called"));
#endif
    current_position = 0;
    target_position = 0;
    this->hard_stop();
    homing = false;
    stepdir = 0;
}

void MotorTask::set_max_speed(uint16_t speed)
{
    max_speed = speed;
    if (max_speed > MAX_PPS)
    {
        max_speed = MAX_PPS;
    }
}

void MotorTask::set_target_speed(uint16_t speed)
{
    target_speed = speed;
    if (target_speed > max_speed)
    {
        target_speed = max_speed;
    }
    if (target_speed > 0 && target_speed < MIN_PPS)
    {
        target_speed = MIN_PPS;
    }
}


void MotorTask::accelerate(void)
{
    uint16_t set_current_speed;
    // limit the target speed to max we can manage with HW
    // We should be moving at min speed if at all
    if (current_speed < MIN_PPS && target_speed > 0)
    {
        set_current_speed = MIN_PPS;
    }
    // When accelerating double speed untill at target
    if (current_speed < target_speed)
    {
        uint32_t sanity_check = current_speed * 2;
        if (sanity_check > target_speed)
        {
            set_current_speed = target_speed;
        }
        else
        {
            set_current_speed = sanity_check;
        }
    }
    // When decelerating halve difference until at snap-limit
    if (current_speed > target_speed)
    {
        set_current_speed = current_speed - (current_speed - target_speed)/2;
        if (set_current_speed - target_speed < PPS_SNAP_LIMIT)
        {
            set_current_speed = target_speed;
        }
    }

#if defined(DEBUG_SERIAL) && defined(VERBOSE_DEBUG)
    DEBUG_SERIAL.print(F("current_speed="));
    DEBUG_SERIAL.println(current_speed, DEC);
    DEBUG_SERIAL.print(F("set_current_speed="));
    DEBUG_SERIAL.println(set_current_speed, DEC);
#endif
    if (set_current_speed == 0)
    {
#if defined(DEBUG_SERIAL)
        DEBUG_SERIAL.println("speed 0 requested, going for stop");
#endif
        this->hard_stop();
    }
    else
    {
        if (current_speed == 0)
        {
            this->start_stepping();
        }
        this->set_pps(set_current_speed);
    }
}

void MotorTask::hard_stop(void)
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("hard_stop called"));
#endif
    target_speed = 0;
    Timer1.stop();
    digitalWrite(RDY_PIN, LOW);
    current_speed = 0;
    // Disable the driver
    FastGPIO::Pin<STEPPER_PINS[2]>::setOutputValueHigh();
    target_position = current_position;
    if (stop_callback)
    {
        this->stop_callback();
    }
}

void MotorTask::set_pps(uint16_t pps)
{
#if defined(DEBUG_SERIAL) && defined(VERBOSE_DEBUG)
    DEBUG_SERIAL.println(F("set_pps called"));
#endif
    current_speed = pps;
    Timer1.setPeriod(1000000 / pps);
    Timer1.pwm(STEPPER_PINS[0], 1023/2);
}

void MotorTask::start_stepping(void)
{
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("start_stepping called"));
#endif
    if (target_position == current_position && !homing)
    {
#ifdef DEBUG_SERIAL
        DEBUG_SERIAL.println(F("target_position == current_position, abort!"));
#endif
        return;
    }
    this->set_pps(MIN_PPS);
    // Enable the driver
    FastGPIO::Pin<STEPPER_PINS[2]>::setOutputValueLow();
    // Set direction
    if (stepdir > 0)
    {
        FastGPIO::Pin<STEPPER_PINS[1]>::setOutputValueHigh();
    }
    else
    {
        FastGPIO::Pin<STEPPER_PINS[1]>::setOutputValueLow();
    }
    Timer1.initialize(1000000 / current_speed);
    Timer1.attachInterrupt(countstep);
    Timer1.pwm(STEPPER_PINS[0], 1023/2);
    digitalWrite(RDY_PIN, HIGH);
}

void MotorTask::run(uint32_t now)
{
    if (!homing)
    {
        if (   stepdir < 0 && current_position <= target_position
            || stepdir > 0 && current_position >= target_position)
        {
            // time to stop
#if defined(DEBUG_SERIAL)
            DEBUG_SERIAL.println(F("Reached target"));
#endif
            this->hard_stop();
        }
        // Hit home switch
        if (stepdir < 1 && !homesw.read())
        {
#if defined(DEBUG_SERIAL)
            DEBUG_SERIAL.println(F("Home switch hit!"));
#endif
            this->hard_stop();
        }
    }
    if ((now - last_speed_adjust) > SPEED_ADJUST_INTERVAL)
    {
#if defined(DEBUG_SERIAL) && defined(VERBOSE_DEBUG)
        DEBUG_SERIAL.print(F("current_position="));
        DEBUG_SERIAL.println(current_position, DEC);
        DEBUG_SERIAL.print(F("target_position="));
        DEBUG_SERIAL.println(target_position, DEC);
#endif
        last_speed_adjust = now;
        if (!homing)
        {
            uint32_t dtg;
            if (current_position < target_position)
            {
                dtg = target_position - current_position;
            }
            else
            {
                dtg = current_position - target_position;
            }
            // TODO: figure out a better target deccel algo
            uint16_t speedrequest;
            if (dtg > MAX_PPS)
            {
                speedrequest = MAX_PPS;
            }
            else
            {
                speedrequest = dtg*DTG_SPEED_FACTOR;
            }
            this->set_target_speed(speedrequest);
        }
        if (target_speed != current_speed)
        {
            this->accelerate();
        }
    }
}

MotorTask motortask;
extern MotorTask motortask;

void countstep(void)
{
    motortask.current_position = motortask.current_position + motortask.stepdir;
    motortask.stepped = true;
}


#endif
