#ifndef QUICK_SETTINGS_HPP
#define QUICK_SETTINGS_HPP

#include "Pattern.hpp"

class QuickSettings : public Pattern
{
public:
    QuickSettings()
    {
        currentPalette = acid_gp;
    }
    bool RunPattern() override;

    void SetLed(uint8_t x, uint8_t y, bool state = true) override
    {
    }

    void SetOption(uint8_t option, uint8_t amount = 4) override
    {
        selectedOption = option;
        optionAmount = amount;
    }

    void SetValue(uint8_t value, uint8_t amount) override
    {
        selectedValue = value;
        valueAmount = amount;
    }

private:

    uint8_t colorIndex = 255;
};

bool QuickSettings::RunPattern()
{
    fill_solid(patternleds, 16, CRGB::Black);
    CRGB optionColor = ColorFromPalette(currentPalette, 1, 255, LINEARBLEND_NOWRAP);
    CRGB optionDimColor = ColorFromPalette(currentPalette, 40, 20, LINEARBLEND_NOWRAP);

    CRGB valueColor = ColorFromPalette(currentPalette, 255, 255, LINEARBLEND_NOWRAP);
    CRGB valueDimColor = ColorFromPalette(currentPalette, 160, 20, LINEARBLEND_NOWRAP);

    for (uint8_t i = 0; i < 4; i++)
    {
        if (i == selectedOption)
        {
            patternleds[i] = optionColor;
        }
        else
        {
            patternleds[i] = optionDimColor;
        }
    }

    // for this we will need also the value amount, and we offset the leds so that the last value is always at the end

    for (uint8_t i = 0; i < 12; i++)
    {
        if (i < valueAmount)
        {
            if (i == selectedValue)
            {
                patternleds[4 + i] = valueColor;
            }
            else
            {
                patternleds[4 + i] = valueDimColor;
            }
        }
    }
    return true;
}

#endif // QUICKSETTINGS_HPP
