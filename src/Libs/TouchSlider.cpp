#include "TouchSlider.hpp"

bool CapTouch::Init(uint8_t pin)
{
    _pin = pin;
    pinMode(_pin, INPUT);

    const int maxAttempts = 5;
    const float maxThreshold = 50000.0f;
    int attempts = 0;

    do
    {
        // Reset raw value for each attempt
        raw = 0;

        // Initialize history and smoothed value to an average of a few readings
        for (int i = 0; i < 10; i++)
        {
            raw += touchRead(_pin);
            delay(1);
        }
        raw = raw / 10;

        attempts++;

        if (raw < maxThreshold)
        {
            // Valid reading, break the loop
            break;
        }
        else
        {
            // Reading too high, log a warning and retry
            log_w("Pin %d: Initial touch value too high (%.2f). Retrying... (Attempt %d/%d)",
                  _pin, raw, attempts, maxAttempts);
            delay(100); // Add a small delay before retrying
        }
    } while (attempts < maxAttempts);

    if (attempts >= maxAttempts)
    {
        log_e("Pin %d: Failed to get a valid initial touch value after %d attempts. Using last reading: %.2f",
              _pin, maxAttempts, raw);
        return false;
    }

    log_d("Pin: %d, Raw: %.2f", _pin, raw);
    p3 = raw;
    p2 = raw;
    p1 = raw;
    smoothed = raw;
    baseline = raw;
    return true;
}

int CapTouch::GetValue()
{
    raw = touchRead(_pin);

    p1 = raw; // Latest point in the history

    // Glitch detector
    if (abs(p3 - p1) < 5)
    { // The latest point and the two points back are pretty close
        if (abs(p2 - p3) > 3)
        { // The point in the middle is too different from the adjacent points -- ignore
            p2 = p3;
        }
    }

    // Smooth the de-glitched data to take out some noise
    smoothed = p3 * (1 - dataSmoothingFactor) + smoothed * dataSmoothingFactor;

    // Dynamic baseline tracking -- a much longer view of the de-glitched data
    if (count > 50)
    {
        baseline = p3 * (1 - baselineSmoothingFactor) + baseline * baselineSmoothingFactor;
    }

    // Shift the history
    p3 = p2;
    p2 = p1;
    // Replace this with code to read the actual sensor value
    _lastValue = smoothed - baseline;
    if (_lastValue < _threshold)
    {
        _lastValue = 0;
    }
    return _lastValue;
}
////////////////////////////////////////////

bool TouchSlider::Init(uint8_t *gpio)
{
    for (uint8_t i = 0; i < NUM_SENSORS; i++)
    {
        if (!t[i].Init(gpio[i]))
        {
            return false;
        }
    }
    timer.Restart();
    log_d("TouchSlider initialized");
    return true;
};

uint8_t TouchSlider::GetQuantizedPosition(uint8_t numPositions)
{
    // This function now works with any number of positions by dividing the range [0, 1] into equal intervals
    if (numPositions == 0)
        return 0; // Guard against division by zero if no positions are specified

    float interval = 1.0f / numPositions;
    uint8_t quantized_position = static_cast<uint8_t>(lastPosition / interval);

    // Ensure that the maximum index does not exceed numPositions - 1
    if (quantized_position >= numPositions)
    {
        quantized_position = numPositions - 1;
    }

    return quantized_position;
}

void TouchSlider::SetPosition(uint8_t intPosition, uint8_t numPositions)
{
    float interval = 1.0f / numPositions;
    uint8_t quantized_position = static_cast<uint8_t>(intPosition / interval);
}

bool TouchSlider::ReadValues()
{
    // Read values from sensors
    for (int i = 0; i < NUM_SENSORS; i++)
    {
        sensorValues[i] = ReadSensorValue(i);
    }

    // Find the sensor with the highest value
    int maxValue = 0;
    int maxSensor = 0;
    for (int i = 0; i < NUM_SENSORS; i++)
    {
        if (sensorValues[i] > maxValue)
        {
            maxValue = sensorValues[i];
            maxSensor = i;
        }
    }

    if (maxValue >= touchThreshold)
    {
        timer.CalculateDeltaTime();
        int32_t deltaTime = timer.GetDeltaTime();
        // Approximate position using linear interpolation
        int nextSensor = maxSensor == NUM_SENSORS - 1 ? maxSensor : maxSensor + 1;
        int prevSensor = maxSensor == 0 ? maxSensor : maxSensor - 1;

        float sum = sensorValues[maxSensor] + sensorValues[prevSensor] + sensorValues[nextSensor];

        // Proportions
        float c1 = maxSensor == 0 ? 0 : sensorValues[prevSensor] / sum;
        float c2 = sensorValues[maxSensor] / sum;
        float c3 = maxSensor == NUM_SENSORS - 1 ? 0 : sensorValues[nextSensor] / sum;

        // Offset
        float offset = 0.0f;
        if (c1 > c3)
        {
            offset = -c1;
        }
        else
        {
            offset = c3;
        }

        float offsetPad = static_cast<float>(maxSensor) + offset; // Offset relative to pad
        float position = offsetPad / (NUM_SENSORS - 1);           // Adjusted for slider (0 to 1)

        position = constrain(position * 1.04f - 0.02f, 0.0f, 1.0f);

        if (!touched)
        {
            touched = true;
            startPosition = position;
            lastPosition = position;
        }

        // Calculate direction and speed of movement
        if (position > lastPosition)
        {
            direction = 1;
            speed = ((position - lastPosition) / deltaTime) * 20.0f;
        }
        else if (position < lastPosition)
        {
            direction = -1;
            speed = ((lastPosition - position) / deltaTime) * 20.0f;
        }
        else
        {
            direction = 0;
            speed = 0;
        }

        distance += speed * deltaTime;

        if (speed != 0)
        {
            lastPosition = position;
        }
    }

    else
    {
        touched = false;
        speed = 0.0f;
        distance = 0.0f;
    }

    return touched;
}