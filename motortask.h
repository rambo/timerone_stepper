#ifndef MOTORTASKS_H
#define MOTORTASKS_H

#include <Arduino.h>
#include <Task.h>
#include <FastGPIO.h>
#include <TimerOne.h>

#define SPEED_ADJUST_INTERVAL 50
#define MIN_PPS 5
#define PPS_SNAP_LIMIT 20
#define MAX_PPS (USTEP_FACTOR * STEPS_PER_REV)


void countstep(void);

class MotorTask : public Task
{
public:
    MotorTask();
    virtual void run(uint32_t now);
    virtual bool canRun(uint32_t now);
    void start();
    void hard_stop();
    void home();
    void home_done();
    void set_pps(uint16_t pps);
    void accelerate();
    void set_target_speed(uint16_t speed);
    void set_position(uint16_t tgt_position);
    volatile bool stepped;
    volatile uint16_t current_position;
    bool homing;
    int8_t stepdir;
    uint16_t target_position;
    uint32_t last_speed_adjust;
    uint16_t target_speed = MAX_PPS;
    uint16_t current_speed;
};

MotorTask::MotorTask()
: Task()
{
    Timer1.initialize(150000);
    Timer1.pwm(STEPPER_PINS[0], 1023/2);
    Timer1.attachInterrupt(countstep);
    Timer1.stop();
}


// We can't just return true since then no other task could ever run (since we have the priority)
bool MotorTask::canRun(uint32_t now)
{
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

void MotorTask::set_position(uint16_t tgt_position)
{
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

void MotorTask::home(void)
{
    target_position = 0;
    homing = true;
    stepdir = -1;
}

void MotorTask::home_done(void)
{
    homing = false;
    stepdir = 0;
    current_position = 0;
}

void MotorTask::set_target_speed(uint16_t speed)
{
    target_speed = speed;
    if (target_speed > MAX_PPS)
    {
        target_speed = MAX_PPS;
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
    if (set_current_speed == 0)
    {
        this->hard_stop();
    }
    else
    {
        if (current_speed == 0)
        {
            this->start();
        }
        this->set_pps(set_current_speed);
    }
}

void MotorTask::hard_stop(void)
{
    target_speed = 0;
    Timer1.stop();
    current_speed = 0;
    // Disable the driver
    FastGPIO::Pin<STEPPER_PINS[2]>::setOutputValueHigh();
}

void MotorTask::set_pps(uint16_t pps)
{
    current_speed = pps;
    last_speed_adjust = millis();
    Timer1.setPeriod(1000000 / pps);
}

void MotorTask::start(void)
{
    this->set_pps(MIN_PPS);
    // Enable the driver
    FastGPIO::Pin<STEPPER_PINS[2]>::setOutputValueHigh();
    // Set direction
    if (stepdir > 0)
    {
        FastGPIO::Pin<STEPPER_PINS[1]>::setOutputValueHigh();
    }
    else
    {
        FastGPIO::Pin<STEPPER_PINS[1]>::setOutputValueLow();
    }
    Timer1.restart();
}

void MotorTask::run(uint32_t now)
{
    if (!homing)
    {
        if (   stepdir < 0 && current_position <= target_position
            || stepdir > 1 && current_position >= target_position)
        {
            this->hard_stop();
        }
    }
    if ((now - last_speed_adjust) > SPEED_ADJUST_INTERVAL)
    {
        uint16_t dtg;
        if (current_position < target_position)
        {
            dtg = target_position - current_position;
        }
        else
        {
            dtg = current_position - target_position;
        }
        // TODO: figure out a better accel/deccel algo
        this->set_target_speed(dtg);
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
