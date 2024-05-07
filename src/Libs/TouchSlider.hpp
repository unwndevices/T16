#ifndef TOUCHSLIDER_HPP
#define TOUCHSLIDER_HPP
#include <Arduino.h>
#include "Timer.hpp"
#include "Signal.hpp"

#define NUM_SENSORS 7

class CapTouch
{
public:
    CapTouch(){};
    CapTouch(uint8_t pin) { _pin = pin; };

    void Init(uint8_t pin);
    int GetValue();
    bool IsPressed() { return _pressed; };
    void SetThreshold(uint16_t threshold) { _threshold = threshold; };

private:
    // HW
    uint8_t _pin = 0;

    // SMOOTHING
    float p1 = 0.0, p2 = 0.0, p3 = 0.0; // 3-Point history
    float raw = 0.0;                    // Current reading
    float baseline = 0.0;
    float smoothed = 0.0;
    unsigned long count = 0;

    // Smoothing factors. The closer to one (1.0) the smoother the data. Smoothing
    // introduces a delay.
    const float dataSmoothingFactor = 0.75;
    const float baselineSmoothingFactor = 0.9995;

    // state
    uint16_t _threshold = 600;
    bool _pressed = false;
    int _lastValue = 0;
};

class TouchSlider
{
public:
    TouchSlider(){};

    CapTouch t[NUM_SENSORS]; // Change this to match your slider
    void Init(uint8_t *gpio);
    void SetPosition(float position) { lastPosition = position; };
    void SetPosition(uint8_t intPosition, uint8_t numPositions);
    float GetPosition() { return lastPosition; };
    uint8_t GetQuantizedPosition(uint8_t numPositions);
    float GetSpeed() { return speed * (float)direction; };
    bool ReadValues();

    void Update()
    {
        float prevPosition = GetPosition();
        ReadValues();

        for (uint8_t i = 0; i < NUM_SENSORS; i++)
        {
            bool currentState = IsTouched(i);

            if (currentState && !prevSensorState[i])
            {
                onSensorTouched.Emit(i, true);
            }
            else if (!currentState && prevSensorState[i])
            {
                onSensorTouched.Emit(i, false);
            }

            prevSensorState[i] = currentState;
        }
    }

    bool IsTouched(float threshold = 0.0f)
    {
        if (threshold == 0.0f)
            return touched;
        else if (distance > threshold)
        {
            return true;
        }
        else
            return false;
    };

    int sensorValues[NUM_SENSORS];
    int direction;
    float speed;

    bool IsTouched(uint8_t sensorNum, uint16_t threshold = 12000)
    {

        return t[sensorNum].GetValue() > threshold;
    }

    Signal<uint8_t, bool> onSensorTouched;

private:
    int touchThreshold = 12000;
    bool touched = false;
    float distance = 0.0f;
    float startPosition = 0.0f;
    float endPosition = 0.0f;
    float lastPosition = 0.0f;

    bool clickDetected = false;
    unsigned long pressTime = 0;
    unsigned long releaseTime = 0;
    const unsigned long clickTime = 300;
    bool prevSensorState[NUM_SENSORS] = {false};

    Timer timer;
    int ReadSensorValue(int sensorNum) { return t[sensorNum].GetValue(); };
};

#endif // TOUCHSLIDER_HPP
