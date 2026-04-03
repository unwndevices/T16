#include "Arduino.h"

// Include implementation sources for native test linking
#include "../native_stubs/Arduino.cpp"

#include <unity.h>
#include "../../src/Libs/MidiTransport.hpp"

// Mock transport implementing the MidiTransport interface
class MockTransport : public t16::MidiTransport
{
public:
    int noteOnCount = 0;
    int noteOffCount = 0;
    int ccCount = 0;
    int pitchBendCount = 0;
    int afterTouchCount = 0;
    int sysExCount = 0;
    int readCount = 0;

    uint8_t lastNote = 0;
    uint8_t lastVelocity = 0;
    uint8_t lastChannel = 0;
    uint8_t lastCC = 0;
    uint8_t lastCCValue = 0;
    int lastBend = 0;
    uint8_t lastPressure = 0;
    size_t lastSysExSize = 0;

    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) override
    {
        noteOnCount++;
        lastNote = note;
        lastVelocity = velocity;
        lastChannel = channel;
    }

    void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) override
    {
        noteOffCount++;
        lastNote = note;
        lastVelocity = velocity;
        lastChannel = channel;
    }

    void sendControlChange(uint8_t cc, uint8_t value, uint8_t channel) override
    {
        ccCount++;
        lastCC = cc;
        lastCCValue = value;
        lastChannel = channel;
    }

    void sendPitchBend(int bend, uint8_t channel) override
    {
        pitchBendCount++;
        lastBend = bend;
        lastChannel = channel;
    }

    void sendAfterTouch(uint8_t note, uint8_t pressure, uint8_t channel) override
    {
        afterTouchCount++;
        lastNote = note;
        lastPressure = pressure;
        lastChannel = channel;
    }

    void sendSysEx(size_t size, const byte* data) override
    {
        sysExCount++;
        lastSysExSize = size;
    }

    void read() override
    {
        readCount++;
    }

    void setHandleSystemExclusive(void (*function)(byte*, unsigned)) override
    {
        // No-op for mock
    }
};

static MockTransport mock;

void setUp(void)
{
    mock = MockTransport();
}

void tearDown(void) {}

void test_mock_transport_receives_note_on(void)
{
    mock.sendNoteOn(60, 100, 1);
    TEST_ASSERT_EQUAL(1, mock.noteOnCount);
    TEST_ASSERT_EQUAL(60, mock.lastNote);
    TEST_ASSERT_EQUAL(100, mock.lastVelocity);
    TEST_ASSERT_EQUAL(1, mock.lastChannel);
}

void test_mock_transport_receives_note_off(void)
{
    mock.sendNoteOff(60, 0, 1);
    TEST_ASSERT_EQUAL(1, mock.noteOffCount);
    TEST_ASSERT_EQUAL(60, mock.lastNote);
    TEST_ASSERT_EQUAL(0, mock.lastVelocity);
    TEST_ASSERT_EQUAL(1, mock.lastChannel);
}

void test_mock_transport_receives_sysex(void)
{
    byte data[] = {0xF0, 0x7E, 0x06, 0x01, 0xF7};
    mock.sendSysEx(5, data);
    TEST_ASSERT_EQUAL(1, mock.sysExCount);
    TEST_ASSERT_EQUAL(5, mock.lastSysExSize);
}

void test_mock_transport_receives_cc(void)
{
    mock.sendControlChange(1, 64, 1);
    TEST_ASSERT_EQUAL(1, mock.ccCount);
    TEST_ASSERT_EQUAL(1, mock.lastCC);
    TEST_ASSERT_EQUAL(64, mock.lastCCValue);
}

void test_mock_transport_receives_pitch_bend(void)
{
    mock.sendPitchBend(8192, 1);
    TEST_ASSERT_EQUAL(1, mock.pitchBendCount);
    TEST_ASSERT_EQUAL(8192, mock.lastBend);
}

void test_mock_transport_receives_aftertouch(void)
{
    mock.sendAfterTouch(60, 80, 1);
    TEST_ASSERT_EQUAL(1, mock.afterTouchCount);
    TEST_ASSERT_EQUAL(60, mock.lastNote);
    TEST_ASSERT_EQUAL(80, mock.lastPressure);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_mock_transport_receives_note_on);
    RUN_TEST(test_mock_transport_receives_note_off);
    RUN_TEST(test_mock_transport_receives_sysex);
    RUN_TEST(test_mock_transport_receives_cc);
    RUN_TEST(test_mock_transport_receives_pitch_bend);
    RUN_TEST(test_mock_transport_receives_aftertouch);
    return UNITY_END();
}
