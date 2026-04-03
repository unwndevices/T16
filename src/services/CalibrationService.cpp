#include "CalibrationService.hpp"
#include "../Libs/Adc.hpp"
#include "../Libs/Leds/LedManager.hpp"
#include "../Libs/Button.hpp"
#include "../Libs/TouchSlider.hpp"
#include "../Libs/DataManager.hpp"
#include <FastLED.h>

namespace t16
{

CalibrationService::CalibrationService(Adc& adc, LedManager& leds)
    : adc_(adc)
    , ledManager_(leds)
{
}

void CalibrationService::runHardwareTest(Key* keys, uint8_t numKeys)
{
    ledManager_.TestAll(HUE_YELLOW);
    delay(1500);
    ledManager_.OffAll();

    bool anyFailed = false;

    // Test each key with a 3-second timeout per key (FWBUG-04 fix)
    for (uint8_t i = 0; i < numKeys; i++)
    {
        adc_.SetMuxChannel(keys[i].mux_idx);
        uint16_t value = adc_.GetRaw();
        if (value > 4000 || value < 50)
        {
            ledManager_.SetLed(i, true);
            anyFailed = true;
        }
        FastLED.show();
    }

    if (anyFailed)
    {
        // Show failed key positions for 3 seconds then continue
        // (previously this was an infinite while loop -- FWBUG-04)
        unsigned long start = millis();
        while (millis() - start < KEY_TEST_TIMEOUT_MS)
        {
            delay(100);
        }
        ledManager_.OffAll();
    }
}

void CalibrationService::runCalibration(Key* keys, uint8_t numKeys, Button& touchBtn,
                                        Button& modeBtn, Keyboard& keyboard, TouchSlider& slider)
{
    const uint16_t CALIBRATION_DELAY = 5;
    const uint8_t PRESSES_REQUIRED = 4;
    const uint16_t PRESS_THRESHOLD_OFFSET = 1520;

    // Initialize calibration
    modeBtn.onStateChanged.DisconnectAll();
    touchBtn.onStateChanged.DisconnectAll();
    keyboard.RemoveOnStateChanged();
    slider.onSensorTouched.DisconnectAll();

    uint16_t minVal[16], maxVal[16];

    // Calibrate minimum values for all keys
    for (int i = 0; i < numKeys; i++)
    {
        minVal[i] = adc_.CalibrateMin(i);
    }

    delay(500);

    // Calibrate maximum values
    for (int i = 0; i < numKeys; i++)
    {
        ledManager_.SetLed(i, true);
        ledManager_.SetSliderOff();
        FastLED.show();

        uint16_t press_threshold = minVal[keys[i].mux_idx] + PRESS_THRESHOLD_OFFSET;
        uint8_t press_count = 0;
        bool was_pressed = false;
        adc_.SetMuxChannel(keys[i].mux_idx);

        while (press_count < PRESSES_REQUIRED)
        {
            uint16_t filteredValue = adc_.GetRaw();

            if (filteredValue > press_threshold)
            {
                was_pressed = true;
                maxVal[keys[i].mux_idx] = adc_.CalibrateMax(keys[i].mux_idx);
                ledManager_.SetSliderLed(press_count, 255, 2);
                FastLED.show();
            }
            else if (filteredValue <= press_threshold - 300 && was_pressed)
            {
                was_pressed = false;
                press_count++;
            }

            adc_.GetCalibration(minVal, maxVal, numKeys);
            Serial.printf("Key %d. Current: %d, Min: %d, Max: %d, Threshold: %d, Presses: %d\n",
                          i, filteredValue, minVal[keys[i].mux_idx], maxVal[keys[i].mux_idx],
                          press_threshold, press_count);

            delay(CALIBRATION_DELAY);
            yield();
        }

        adc_.GetCalibration(minVal, maxVal, numKeys);
        Serial.printf("Key %d calibrated. Min: %d, Max: %d\n", i, minVal[keys[i].mux_idx],
                      maxVal[keys[i].mux_idx]);
        delay(500);
    }

    // Save calibration data
    uint16_t minVals[16], maxVals[16];
    adc_.GetCalibration(minVals, maxVals, numKeys);

    DataManager calibration("/calibration_data.json");
    calibration.Init();
    calibration.SaveArray(minVals, "minVal", numKeys);
    calibration.SaveArray(maxVals, "maxVal", numKeys);

    // Reset filter window size to default
    adc_.SetFilterWindowSize(16);

    Serial.println("Calibration complete. Restarting...");
    ESP.restart();
}

} // namespace t16
