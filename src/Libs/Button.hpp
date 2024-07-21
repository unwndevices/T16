#ifndef BUTTON_HPP
#define BUTTON_HPP

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

    Button(int pin = 0, int id = 0, int debounceTime = 10)
        : pin(pin),
          id(id),
          debounceTime(debounceTime),
          lastDebounceTime(0),
          state(IDLE), prevState(IDLE),
          pressStartTime(0),
          elapsedTime(0),
          longPressTime(650),
          clickTime(260),
          previousReading(false),
          longPressFlag(false) {}

    void SetLongPressTime(unsigned long time)
    {
        longPressTime = time;
    }

    void Init(int id)
    {
        log_d("Button %d initialized", id);
        this->id = id;
        pinMode(pin, INPUT_PULLUP);
        state = IDLE;
        prevState = IDLE;
        pressStartTime = 0;
        elapsedTime = 0;
        longPressFlag = false;
    }

    void Init()
    {
        pinMode(pin, INPUT_PULLUP);
    }

    void Start()
    {
        xTaskCreatePinnedToCore(Button::taskUpdate, "button", 1024 * 2, this, 3, &_task, 0);
    }

    static void taskUpdate(void *pvParameters)
    {
        Button *button = static_cast<Button *>(pvParameters);
        while (1)
        {
            button->Update();
            vTaskDelay(pdMS_TO_TICKS(4));
        }
    }

    bool GetRaw() { return reading; }

    void Update()
    {
        reading = (bool)(!digitalRead(pin));

        if (reading != previousReading)
        {
            lastDebounceTime = millis();
            previousReading = reading;
            return;
        }

        if ((millis() - lastDebounceTime) > debounceTime)
        {
            switch (state)
            {
            case IDLE:
                if (reading)
                {
                    state = PRESSED;
                    pressStartTime = millis();
                    elapsedTime = 0;
                }
                break;

            case PRESSED:
                if (reading)
                {
                    elapsedTime = millis() - pressStartTime;
                    if (elapsedTime > longPressTime)
                    {
                        state = LONG_PRESSED;
                    }
                }
                else if (!reading)
                {
                    if ((millis() - pressStartTime) < clickTime)
                    {
                        state = CLICKED;
                    }
                    else
                    {
                        state = RELEASED;
                    }
                }
                break;

            case RELEASED:
                if (reading)
                {
                    state = PRESSED;
                    pressStartTime = millis();
                }
                else
                {
                    state = IDLE;
                }
                break;

            case LONG_PRESSED:
                if (!reading)
                {
                    state = LONG_RELEASED;
                }
                break;

            case LONG_RELEASED:
                if (reading)
                {
                    state = PRESSED;
                    pressStartTime = millis();
                }
                else
                {
                    state = IDLE;
                }
                break;

            case CLICKED:
                state = IDLE;
                break;
            }

            if (prevState != state)
            {
                prevState = state;
                onStateChanged.Emit(id, state);
            }
        }
    }

    State GetState()
    {
        return state;
    }

    bool IsPressed()
    {
        return reading;
    }

    float GetHoldTimeNormalized()
    {
        return (float)elapsedTime / (float)longPressTime;
    }

    Signal<int, Button::State> onStateChanged;

private:
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

#endif // !BUTTON_HPP