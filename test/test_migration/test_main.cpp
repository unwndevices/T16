#include "Arduino.h"
#include "LittleFS.h"

// Include implementation sources for native test linking
#include "../native_stubs/Arduino.cpp"
#include "../native_stubs/LittleFS.cpp"
#include "../../src/ConfigManager.cpp"

#include <unity.h>
#include "ConfigManager.hpp"

static ConfigManager cm;

static const char* V103_JSON =
    "{\"version\":103,\"mode\":2,\"sensitivity\":1,\"brightness\":4,"
    "\"midi_trs\":1,\"trs_type\":0,\"passthrough\":1,\"midi_ble\":0,"
    "\"custom_scale1\":[0,2,4,5,7,9,11,12,0,2,4,5,7,9,11,12],"
    "\"custom_scale2\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],"
    "\"banks\":["
    "{\"pal\":0,\"ch\":1,\"scale\":3,\"oct\":2,\"note\":0,\"vel\":1,"
    "\"at\":1,\"flip_x\":0,\"flip_y\":0,\"koala_mode\":0,"
    "\"chs\":[1,1,1,1,1,1,1,1],\"ids\":[13,14,15,1,16,17,18,19]},"
    "{\"pal\":1,\"ch\":2,\"scale\":0,\"oct\":3,\"note\":0,\"vel\":1,"
    "\"at\":1,\"flip_x\":0,\"flip_y\":0,\"koala_mode\":0,"
    "\"chs\":[2,2,2,2,2,2,2,2],\"ids\":[20,21,22,23,24,25,26,27]},"
    "{\"pal\":2,\"ch\":3,\"scale\":0,\"oct\":2,\"note\":0,\"vel\":1,"
    "\"at\":1,\"flip_x\":0,\"flip_y\":0,\"koala_mode\":0,"
    "\"chs\":[1,1,1,1,1,1,1,1],\"ids\":[13,14,15,1,16,17,18,19]},"
    "{\"pal\":3,\"ch\":4,\"scale\":0,\"oct\":2,\"note\":0,\"vel\":1,"
    "\"at\":1,\"flip_x\":0,\"flip_y\":0,\"koala_mode\":0,"
    "\"chs\":[1,1,1,1,1,1,1,1],\"ids\":[13,14,15,1,16,17,18,19]}"
    "]}";

static void write_v103_config(void)
{
    LittleFS.begin();
    auto f = LittleFS.open("/configuration_data.json", "w", true);
    if (f)
    {
        f.write((const uint8_t*)V103_JSON, strlen(V103_JSON));
        f.close();
    }
}

void setUp(void)
{
    LittleFS.cleanup();
    LittleFS.begin();
    set_fake_millis(0);
    cm = ConfigManager(); // Reset to clean state
}

void tearDown(void)
{
    LittleFS.cleanup();
}

// --- Migration from v103 to v200 ---

void test_version_migrated_to_200(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(200, cm.Global().version);
}

void test_mode_preserved(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(2, cm.Global().mode);
}

void test_sensitivity_preserved(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(1, cm.Global().sensitivity);
}

void test_brightness_preserved(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(4, cm.Global().brightness);
}

void test_midi_trs_preserved(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(1, cm.Global().midi_trs);
}

void test_passthrough_preserved(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(1, cm.Global().passthrough);
}

void test_midi_ble_preserved(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(0, cm.Global().midi_ble);
}

void test_bank0_scale_preserved(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(3, cm.Bank(0).scale);
}

void test_bank1_channel_preserved(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(2, cm.Bank(1).channel);
}

void test_custom_scale1_preserved(void)
{
    write_v103_config();
    cm.Init();
    // custom_scale1 should be [0,2,4,5,7,9,11,12,0,2,4,5,7,9,11,12]
    TEST_ASSERT_EQUAL_INT8(0, cm.Global().custom_scale1[0]);
    TEST_ASSERT_EQUAL_INT8(2, cm.Global().custom_scale1[1]);
    TEST_ASSERT_EQUAL_INT8(4, cm.Global().custom_scale1[2]);
    TEST_ASSERT_EQUAL_INT8(5, cm.Global().custom_scale1[3]);
    TEST_ASSERT_EQUAL_INT8(7, cm.Global().custom_scale1[4]);
    TEST_ASSERT_EQUAL_INT8(12, cm.Global().custom_scale1[7]);
}

void test_serialized_output_has_global_key(void)
{
    write_v103_config();
    cm.Init();
    char buffer[2048];
    cm.SerializeToBuffer(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\"global\""));
}

// --- v200 passthrough ---

void test_v200_passes_through(void)
{
    // Write a v200 config
    const char* v200 =
        "{\"global\":{\"version\":200,\"mode\":1,\"sensitivity\":2,"
        "\"brightness\":5,\"midi_trs\":0,\"trs_type\":1,"
        "\"passthrough\":0,\"midi_ble\":1,"
        "\"custom_scale1\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],"
        "\"custom_scale2\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]},"
        "\"banks\":[]}";
    LittleFS.begin();
    auto f = LittleFS.open("/configuration_data.json", "w", true);
    if (f)
    {
        f.write((const uint8_t*)v200, strlen(v200));
        f.close();
    }
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(200, cm.Global().version);
    TEST_ASSERT_EQUAL_UINT8(1, cm.Global().mode);
    TEST_ASSERT_EQUAL_UINT8(5, cm.Global().brightness);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_version_migrated_to_200);
    RUN_TEST(test_mode_preserved);
    RUN_TEST(test_sensitivity_preserved);
    RUN_TEST(test_brightness_preserved);
    RUN_TEST(test_midi_trs_preserved);
    RUN_TEST(test_passthrough_preserved);
    RUN_TEST(test_midi_ble_preserved);
    RUN_TEST(test_bank0_scale_preserved);
    RUN_TEST(test_bank1_channel_preserved);
    RUN_TEST(test_custom_scale1_preserved);
    RUN_TEST(test_serialized_output_has_global_key);
    RUN_TEST(test_v200_passes_through);
    return UNITY_END();
}
