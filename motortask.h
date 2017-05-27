#ifndef MOTORTASKS_H

#define SPEED_ADJUST_INTERVAL 150

class MotorTask : public Task
{
public:
    MotorTask();
    volatile bool stepped;
    volatile uint16_t current_position;
    int8_t stepdir = 1;
    uint16_t target_position;
    uint32_t last_speed_adjust;
    uint16_t target_speed;
    uint16_t current_speed;
}

MotorTask::MotorTask()
: Task()
{
    Timer1.initialize(150000);
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
    return false
}

void MotorTask::run(uint32_t now)
{
    
}

MotorTask motortask;
extern MotorTask motortask;


#endif
