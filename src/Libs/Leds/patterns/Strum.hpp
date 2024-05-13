#ifndef STRUM_HPP
#define STRUM_HPP

#include "Pattern.hpp"

class Strum : public Pattern
{
public:
    Strum()
    {
        currentPalette = topo_gp;
    }
    bool RunPattern() override;

    void SetLed(uint8_t x, uint8_t y, bool state = true) override
    {
    }

    void SetNote(uint8_t note)
    {
        selectedNote = note;
    }

    void SetChord(uint8_t chord)
    {
        selectedChord = chord;
    }

private:
    uint8_t selectedNote = 0;
    uint8_t selectedChord = 0;
    uint8_t colorIndex = 255;
};

bool Strum::RunPattern()
{
    fill_solid(patternleds, 16, CRGB::Black);
    CRGB noteColor = ColorFromPalette(currentPalette, colorIndex, 255, LINEARBLEND_NOWRAP);
    CRGB noteDimColor = ColorFromPalette(currentPalette, colorIndex, 30, LINEARBLEND_NOWRAP);

    CRGB chordColor = ColorFromPalette(currentPalette, 1, 255, LINEARBLEND_NOWRAP);
    CRGB chordDimColor = ColorFromPalette(currentPalette, 1, 30, LINEARBLEND_NOWRAP);

    for (int i = 0; i < 16; i++)
    {
        if (i < 12)
        {
            if (i == selectedNote)
            {
                patternleds[i] = noteColor;
            }
            else
            {
                patternleds[i] = noteDimColor;
            }
        }
        else
        {
            if (selectedChord == i - 12)
            {
                patternleds[i] = chordColor;
            }
            else
            {
                patternleds[i] = chordDimColor;
            }
        }
    }
    return true;
}

#endif // STRUM_HPP
