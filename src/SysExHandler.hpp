#pragma once
#include <Arduino.h>

class ConfigManager;
class MidiProvider;

class SysExHandler
{
public:
    SysExHandler(ConfigManager& configManager, MidiProvider& midiProvider);

    // Main entry point -- called from MidiProvider's SysEx callback
    void ProcessSysEx(byte* data, unsigned length);

private:
    ConfigManager& configManager_;
    MidiProvider& midiProvider_;

    // Firmware version -- read from config at construction
    uint8_t firmwareVersion_ = 0;

    void HandleVersionRequest();
    void HandleConfigDumpRequest();
    void HandleConfigLoad(const byte* payload, size_t payloadLen);
    void HandleParamSet(const byte* payload, size_t payloadLen);
    void HandleCalibrationReset();
    void HandleFactoryReset();
    void HandleBootloaderRequest();
    void HandleCapabilitiesRequest();
    void SendAck(uint8_t cmd, uint8_t status);
};
