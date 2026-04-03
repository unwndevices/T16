#include "SysExHandler.hpp"
#include "SysExProtocol.hpp"
#include "ConfigManager.hpp"
#include "Configuration.hpp"
#include "Libs/MidiProvider.hpp"

SysExHandler::SysExHandler(ConfigManager& configManager, MidiProvider& midiProvider)
    : configManager_(configManager)
    , midiProvider_(midiProvider)
{
    firmwareVersion_ = configManager_.Global().version;
}

void SysExHandler::ProcessSysEx(byte* data, unsigned length)
{
    // Debug log for verifying byte layout from arduino_midi_library
    // data[0] = 0xF0, data[1] = manufacturer ID, data[2] = cmd, data[3] = sub
    log_d("SysEx: len=%u data[0]=0x%02X data[1]=0x%02X data[2]=0x%02X data[3]=0x%02X",
          length, data[0], data[1], data[2], data[3]);

    // PROTO-04: Validate minimum message length
    if (length < SysEx::MIN_MESSAGE_LENGTH)
    {
        log_d("SysEx too short: %u bytes", length);
        return;
    }

    // PROTO-04: Validate manufacturer ID -- silently ignore messages not for us
    if (data[1] != SysEx::MANUFACTURER_ID)
    {
        return;
    }

    uint8_t cmd = data[2];
    uint8_t sub = data[3];
    const byte* payload = data + 4;
    size_t payloadLen = (length > 4) ? length - 4 : 0;

    switch (cmd)
    {
        case SysEx::CMD_VERSION:
            if (sub == SysEx::SUB_REQUEST)
            {
                HandleVersionRequest();
            }
            break;

        case SysEx::CMD_CONFIG:
            if (sub == SysEx::SUB_REQUEST)
            {
                HandleConfigDumpRequest();
            }
            else if (sub == SysEx::SUB_LOAD)
            {
                HandleConfigLoad(payload, payloadLen);
            }
            break;

        case SysEx::CMD_PARAM:
            if (sub == SysEx::SUB_REQUEST)
            {
                HandleParamSet(payload, payloadLen);
            }
            break;

        case SysEx::CMD_CALIBRATION:
            if (sub == SysEx::SUB_REQUEST)
            {
                HandleCalibrationReset();
            }
            break;

        default:
            log_d("Unknown SysEx command: 0x%02X", cmd);
            break;
    }
}

// PROTO-05: Version handshake -- respond with protocol version and firmware version
void SysExHandler::HandleVersionRequest()
{
    byte response[] = {
        SysEx::MANUFACTURER_ID,
        SysEx::CMD_VERSION,
        SysEx::SUB_RESPONSE,
        SysEx::PROTOCOL_VERSION,
        firmwareVersion_
    };
    midiProvider_.SendSysEx(sizeof(response), response);
    log_d("Version response sent: proto=%d fw=%d", SysEx::PROTOCOL_VERSION, firmwareVersion_);
}

// PROTO-03: Config dump -- serialize full config to buffer and send via MIDI
void SysExHandler::HandleConfigDumpRequest()
{
    char buffer[2048];
    buffer[0] = SysEx::MANUFACTURER_ID;
    buffer[1] = SysEx::CMD_CONFIG;
    buffer[2] = SysEx::SUB_RESPONSE;

    size_t json_len = configManager_.SerializeToBuffer(buffer + 3, sizeof(buffer) - 3);
    midiProvider_.SendSysEx(json_len + 3, reinterpret_cast<byte*>(buffer));
    log_d("Config dump sent: %u bytes", json_len);
}

// PROTO-03: Config load -- deserialize full config from editor
void SysExHandler::HandleConfigLoad(const byte* payload, size_t payloadLen)
{
    if (payloadLen == 0)
    {
        log_d("Config load failed: empty payload");
        SendAck(SysEx::CMD_CONFIG, SysEx::STATUS_ERROR);
        return;
    }

    bool ok = configManager_.DeserializeFromBuffer(
        reinterpret_cast<const char*>(payload), payloadLen);

    if (ok)
    {
        SendAck(SysEx::CMD_CONFIG, SysEx::STATUS_OK);
        log_d("Config loaded from editor");
    }
    else
    {
        SendAck(SysEx::CMD_CONFIG, SysEx::STATUS_ERROR);
        log_d("Config load failed: parse error");
    }
}

// PROTO-02: Per-parameter update -- dispatch to ConfigManager setters by domain
void SysExHandler::HandleParamSet(const byte* payload, size_t payloadLen)
{
    // Minimum payload: domain (1) + bankIndex (1) + fieldId (1) + value (1) = 4 bytes
    if (payloadLen < 4)
    {
        log_d("Param set too short: %u bytes", payloadLen);
        return;
    }

    uint8_t domain = payload[0];
    uint8_t bank_index = payload[1];
    uint8_t field_id = payload[2];
    uint8_t value = payload[3];

    switch (domain)
    {
        case SysEx::DOMAIN_GLOBAL:
            configManager_.SetGlobalParam(field_id, value);
            break;

        case SysEx::DOMAIN_BANK_KB:
            if (bank_index >= BANK_AMT)
            {
                log_d("Param set: invalid bank index %d", bank_index);
                return;
            }
            configManager_.SetBankParam(bank_index, field_id, value);
            break;

        case SysEx::DOMAIN_BANK_CC:
            if (bank_index >= BANK_AMT)
            {
                log_d("Param set: invalid bank index %d", bank_index);
                return;
            }
            // CC domain needs 5 bytes: domain + bankIndex + ccIndex + channel + id
            if (payloadLen < 5)
            {
                log_d("CC param set too short: %u bytes", payloadLen);
                return;
            }
            {
                uint8_t cc_index = field_id;
                uint8_t channel = value;
                uint8_t id = payload[4];
                configManager_.SetCCParam(bank_index, cc_index, channel, id);
            }
            break;

        default:
            log_d("Param set: unknown domain 0x%02X", domain);
            return;
    }

    SendAck(SysEx::CMD_PARAM, SysEx::STATUS_OK);
    log_d("Param set: domain=%d bank=%d field=%d value=%d", domain, bank_index, field_id, value);
}

void SysExHandler::HandleCalibrationReset()
{
    log_d("Calibration reset requested");
    // TODO: Wire to CalibrationManager when separated (D-07, Phase 2)
}

void SysExHandler::SendAck(uint8_t cmd, uint8_t status)
{
    byte ack[] = {
        SysEx::MANUFACTURER_ID,
        cmd,
        SysEx::SUB_ACK,
        status
    };
    midiProvider_.SendSysEx(sizeof(ack), ack);
}
