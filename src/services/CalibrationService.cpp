#include "CalibrationService.hpp"
#include "../Libs/Adc.hpp"
#include "../Libs/Leds/LedManager.hpp"
#include "../Libs/Button.hpp"
#include "../Libs/TouchSlider.hpp"
#include "../Libs/DataManager.hpp"
#include "../variant.hpp"
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

    // Test each key with a 3-second timeout per key (FWBUG-04 fix).
    // Multi-mux-aware: drive shared S0..S3 with the channel-within-mux, then
    // sample the owning mux's commonPin (T16: mux_id always 0; T32: keys
    // 16..31 land on mux 1 / GPIO17).
    for (uint8_t i = 0; i < numKeys; i++)
    {
        adc_.SetMuxChannel(keys[i].mux_channel);
        uint16_t value = adc_.GetRawForMux(keys[i].mux_id);
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

    constexpr uint8_t kMaxKeys = variant::CurrentVariant::kConfig.TOTAL_KEYS;
    uint16_t minVal[kMaxKeys], maxVal[kMaxKeys];

    // Indexing convention: `i` and the first arg to CalibrateMin/Max are the
    // *logical key index* (= _channels[] index, matching SetCalibration /
    // GetCalibration which iterate _channels[0..numKeys-1]). `keys[i].mux_id`
    // selects which mux's commonPin to sample, and `keys[i].mux_channel`
    // (0..15) drives the shared S0..S3 select lines via SetMuxChannel.

    // Calibrate minimum values for all keys
    for (int i = 0; i < numKeys; i++)
    {
        adc_.SetMuxChannel(keys[i].mux_channel);
        minVal[i] = adc_.CalibrateMin(i, keys[i].mux_id);
    }

    delay(500);

    // Calibrate maximum values
    for (int i = 0; i < numKeys; i++)
    {
        ledManager_.SetLed(i, true);
        ledManager_.SetSliderOff();
        FastLED.show();

        uint16_t press_threshold = minVal[i] + PRESS_THRESHOLD_OFFSET;
        uint8_t press_count = 0;
        bool was_pressed = false;
        adc_.SetMuxChannel(keys[i].mux_channel);

        while (press_count < PRESSES_REQUIRED)
        {
            uint16_t filteredValue = adc_.GetRawForMux(keys[i].mux_id);

            if (filteredValue > press_threshold)
            {
                was_pressed = true;
                maxVal[i] = adc_.CalibrateMax(i, keys[i].mux_id);
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
                          i, filteredValue, minVal[i], maxVal[i],
                          press_threshold, press_count);

            delay(CALIBRATION_DELAY);
            yield();
        }

        adc_.GetCalibration(minVal, maxVal, numKeys);
        Serial.printf("Key %d calibrated. Min: %d, Max: %d\n", i, minVal[i],
                      maxVal[i]);
        delay(500);
    }

    // Save calibration data
    uint16_t minVals[kMaxKeys];
    uint16_t maxVals[kMaxKeys];
    adc_.GetCalibration(minVals, maxVals, numKeys);

    DataManager calibration(variant::CalibrationFilePath());
    calibration.Init();
    calibration.SaveArray(minVals, "minVal", numKeys);
    calibration.SaveArray(maxVals, "maxVal", numKeys);

    // Reset filter window size to default
    adc_.SetFilterWindowSize(16);

    Serial.println("Calibration complete. Restarting...");
    ESP.restart();
}

} // namespace t16
