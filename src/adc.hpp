#ifndef ADC_HPP
#define ADC_HPP

#include <Arduino.h>
#include <vector>

struct AdcChannelConfig
{
    enum MuxPin
    {
        MUX_SEL_0,
        MUX_SEL_1,
        MUX_SEL_2,
        MUX_SEL_3,
        MUX_SEL_LAST,
    };

    AdcChannelConfig();

    void InitSingle(uint8_t pin);
    void InitMux(uint8_t pin, uint8_t mux_pin_0, uint8_t mux_pin_1 = 0, uint8_t mux_pin_2 = 0, uint8_t mux_pin_3 = 0);

    uint8_t _pin;
    uint8_t _mux_pin[MUX_SEL_LAST];
};

class Adc
{
public:
    struct AdcChannel
    {
        float value = 0.0f;
        uint16_t minVal = 2584;
        uint16_t maxVal = 3770;
        uint16_t buffer[8] = {0};
    };

    Adc();  // constructor
    ~Adc(); // destructor

    void Init(AdcChannelConfig *cfg, uint8_t channels); // initialization method

    void SetCalibration(uint16_t *min, uint16_t *max, uint8_t channels);

    void CalibrationRoutine(); // method to calibrate the ADC
    void CalibrateMin();       // method to calibrate the ADC
    void CalibrateMax();       // method to calibrate the ADC

    void GetCalibration(uint16_t *min, uint16_t *max, uint8_t channels); // method to get the calibration values
    void Start();                                                        // method to start the task (and ADC
    static void Update(void *parameter);                                 // method to update
    void ReadValues();                                                   // method to read the values from the ADC
    float Get(uint8_t chn) const;                                        // method to get the value of a channel as a float
    float GetMux(uint8_t chn, uint8_t index) const;                      // method to get the value of a mux channel as a float

    inline static void fonepole(float &out, float in, float coeff)
    {
        out = (in * coeff) + (out * (1.0f - coeff));
    }

    inline static float slew_limiter(float in, float prev, float slew, unsigned long deltaTime)
    {
        // Calculate the maximum change allowed per millisecond
        float max_change_per_ms = slew * 0.001f;

        // Calculate the maximum change allowed based on the elapsed time
        float max_change = max_change_per_ms * (float)deltaTime;

        // Calculate the difference between the input and previous output
        float delta = in - prev;

        // Limit the change to the maximum allowed
        if (delta > max_change)
        {
            delta = max_change;
        }
        else if (delta < -max_change)
        {
            delta = -max_change;
        }

        // Update the output
        float output = prev + delta;

        return output;
    }

private:
    void SetMuxChannel(uint8_t chn) const; // method to set the mux channel

    AdcChannelConfig _config;
    std::vector<AdcChannel> _channels;
    uint8_t _mux_pin[4];

    TaskHandle_t _task;

    uint16_t AverageValue(uint8_t chn); // method to average the value of a channel
    uint8_t iterator = 0;
    uint8_t avg_iterator = 0;
};
#endif // ADC_HPP
