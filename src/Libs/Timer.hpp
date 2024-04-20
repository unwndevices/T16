#ifndef TIMER_HPP
#define TIMER_HPP
#include <Arduino.h>

class Timer
{
public:
    Timer() : start_time(0),
              current_time(0),
              delta_time(0),
              is_started(false),
              elapsed_time(0){};

    void CalculateDeltaTime()
    {
        ulong now = millis();
        if (is_started)
        {
            delta_time = now - current_time;
            elapsed_time += delta_time;
        }
        current_time = now;
    };

    void AddDeltaTime(uint16_t deltaTime) { elapsed_time += deltaTime; };
    uint16_t GetDeltaTime() { return delta_time; };
    bool IsElapsed(ulong timeout)
    {
        if (is_started && (elapsed_time >= timeout))
        {
            Stop();
            return true;
        }
        return false;
    };

    void Restart()
    {
        is_started = true;
        start_time = millis();
        elapsed_time = 0;
    };

    void Stop()
    {
        is_started = false;
        elapsed_time = 0;
    };

private:
    // time elapsed from last call, in ms
    uint16_t delta_time;
    ulong start_time;
    ulong current_time;
    ulong elapsed_time; // total elapsed time since timer started

    bool is_started;
};

#endif // TIMER_HPP
