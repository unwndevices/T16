#pragma once

#include <Arduino.h>
#include "Signal.hpp"

class Button
{
public:
    enum State
    {
        IDLE,
        PRESSED,
        CLICKED,
        RELEASED,
        LONG_PRESSED,
        LONG_RELEASED,

    };

    Button(int pin = 0, int id = 0, int debounceTime = 10);

    void SetLongPressTime(unsigned long time);
    void Init(int id);
    void Init();
    void Start();
    bool GetRaw();
    void Update();
    State GetState();
    bool IsPressed();
    float GetHoldTimeNormalized();

    Signal<int, Button::State> onStateChanged;

private:
    static void taskUpdate(void *pvParameters);

    int pin, id;
    int debounceTime;
    unsigned long lastDebounceTime;
    State state, prevState;
    bool previousReading, reading;
    unsigned long pressStartTime, elapsedTime;
    unsigned long longPressTime, clickTime;
    bool longPressFlag;
    TaskHandle_t _task;
};
