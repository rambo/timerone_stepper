#ifndef SERIALTASK_H
#define SERIALTASK_H

#include <Arduino.h>
#include <Task.h>
#include "valueparser.h"




#define SERIAL_PARSE_BUFFER_SIZE 27 // 25 + crlf

// We might actually want use the canrun 
class SerialReader : public Task
{
    public:
        // Create a new blinker for the specified pin and rate.
        SerialReader();
        virtual void run(uint32_t now);
        virtual bool canRun(uint32_t now);
        // TODO: control methods for setting desired speeds etc and receiving new-data notifications from the AHRS task

    private:
        void process_command();
        char parsebuffer[SERIAL_PARSE_BUFFER_SIZE+1]; // +1 for null terminator
        uint8_t incoming_position;
};

SerialReader::SerialReader()
: Task()
{
    // Do we need to contruct something ?
    incoming_position = 0;
    parsebuffer[0] = 0x0;
}

bool SerialReader::canRun(uint32_t now)
{
    return Serial.available();
}

void SerialReader::run(uint32_t now)
{
    for (uint8_t d = Serial.available(); d > 0; d--)
    {
        parsebuffer[incoming_position] = Serial.read();
        // Check for line end and in such case do special things
        if (   parsebuffer[incoming_position] == 0xA // LF
            || parsebuffer[incoming_position] == 0xD) // CR
        {
            // Command received, parse it
            parsebuffer[incoming_position] = 0x0; // null-terminate the string
            // Check that we did not miss another CR/LF
            if (   incoming_position > 0
                && (   parsebuffer[incoming_position-1] == 0xD // CR
                    || parsebuffer[incoming_position-1] == 0xA) // LF
               )
            {
                parsebuffer[incoming_position-1] = 0x0; // null-terminate the string
            }
            process_command();
            // Reset position
            incoming_position = 0;
            parsebuffer[0] = 0x0;
            // PONDER: Explicitly clear the buffer with memset ??
            //memset(&parsebuffer, 0x0, sizeof(parsebuffer));
            return;
        }

        incoming_position++;

        // Sanity check buffer sizes
        if (incoming_position > SERIAL_PARSE_BUFFER_SIZE)
        {
            Serial.println(0x15); // NACK
            Serial.print(F("PANIC: No end-of-line seen and incoming_position="));
            Serial.print(incoming_position, DEC);
            Serial.println(F(" clearing buffers"));
            // Reset position
            incoming_position = 0;
            parsebuffer[0]=0x0;
            // PONDER: Explicitly clear the buffer with memset ??
            //memset(&parsebuffer, 0x0, sizeof(parsebuffer));
            return;
        }
    }
}

void SerialReader::process_command()
{
    
    if (parsebuffer[0] == 'G')
    {
        motortask.set_position(ardubus_hex2uint(parsebuffer[1],parsebuffer[2],parsebuffer[3],parsebuffer[4]));
        return;
    }

    // Do we have other command to parse ?

    // If we did not parse a command return NACK
    Serial.println(0x15); // NACK
}

SerialReader serialtask;
extern SerialReader serialtask;



#endif

