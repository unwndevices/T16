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

// v200 fixture with version at the ROOT (correct on-disk format).
// The legacy test_v200_passes_through test had version nested under "global"
// (a fixture quirk that hid a real bug); deleted per Plan 13-03 in favor of
// this root-level fixture.
static const char* V200_JSON_ROOT_VERSION =
    "{\"version\":200,\"global\":{\"mode\":2,\"sensitivity\":1,\"brightness\":4,"
    "\"midi_trs\":1,\"trs_type\":0,\"passthrough\":1,\"midi_ble\":0,"
    "\"custom_scale1\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],"
    "\"custom_scale2\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]},"
    "\"banks\":[{\"pal\":0,\"ch\":1,\"scale\":3,\"oct\":2,\"note\":0,\"vel\":1,"
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
    "\"chs\":[1,1,1,1,1,1,1,1],\"ids\":[13,14,15,1,16,17,18,19]}]}";

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

static void write_config(const char* json)
{
    LittleFS.begin();
    auto f = LittleFS.open("/configuration_data.json", "w", true);
    if (f)
    {
        f.write((const uint8_t*)json, strlen(json));
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

// --- Migration from v103 (chained through v200 → v201) ---

void test_version_migrated_to_201(void)
{
    write_v103_config();
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(201, cm.Global().version);
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

// --- v200 → v201 migration ---

void test_v200_migrated_to_v201(void)
{
    write_config(V200_JSON_ROOT_VERSION);
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(201, cm.Global().version);
    TEST_ASSERT_EQUAL_UINT8(2, cm.Global().mode);          // preserved
    TEST_ASSERT_EQUAL_UINT8(4, cm.Global().brightness);    // preserved
}

void test_v200_migration_serialized_output_has_variant(void)
{
    write_config(V200_JSON_ROOT_VERSION);
    cm.Init();
    char buffer[2048];
    cm.SerializeToBuffer(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\"variant\""));
    // Variant must equal this binary's compile-time variant
    TEST_ASSERT_NOT_NULL(strstr(buffer, variant::CurrentVariant::kConfig.NAME));
}

// --- v201 round-trip + variant gating ---

void test_v201_matching_variant_passes_through(void)
{
    // Build the fixture inline so the variant string matches this binary's variant
    char fixture[2048];
    snprintf(fixture, sizeof(fixture),
        "{\"version\":201,\"variant\":\"%s\","
        "\"global\":{\"mode\":3,\"sensitivity\":2,\"brightness\":7,"
        "\"midi_trs\":0,\"trs_type\":1,\"passthrough\":0,\"midi_ble\":1,"
        "\"custom_scale1\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],"
        "\"custom_scale2\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]},"
        "\"banks\":[]}",
        variant::CurrentVariant::kConfig.NAME);
    write_config(fixture);
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(201, cm.Global().version);
    TEST_ASSERT_EQUAL_UINT8(3, cm.Global().mode);          // preserved (no migration)
    TEST_ASSERT_EQUAL_UINT8(7, cm.Global().brightness);    // preserved
}

void test_v201_variant_mismatch_falls_back_to_defaults(void)
{
    // Fixture claims a variant DIFFERENT from this binary's variant
    const char* otherVariant =
        (strcmp(variant::CurrentVariant::kConfig.NAME, "T16") == 0) ? "T32" : "T16";
    char fixture[2048];
    snprintf(fixture, sizeof(fixture),
        "{\"version\":201,\"variant\":\"%s\","
        "\"global\":{\"mode\":4,\"sensitivity\":3,\"brightness\":8,"
        "\"midi_trs\":1,\"trs_type\":1,\"passthrough\":1,\"midi_ble\":1,"
        "\"custom_scale1\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],"
        "\"custom_scale2\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]},"
        "\"banks\":[]}",
        otherVariant);
    write_config(fixture);
    cm.Init();
    // Defaults from ConfigurationData: mode=0, sensitivity=1, brightness=6
    TEST_ASSERT_EQUAL_UINT8(0, cm.Global().mode);
    TEST_ASSERT_EQUAL_UINT8(1, cm.Global().sensitivity);
    TEST_ASSERT_EQUAL_UINT8(6, cm.Global().brightness);
    TEST_ASSERT_EQUAL_UINT8(201, cm.Global().version);
}

void test_future_version_falls_back_to_defaults(void)
{
    write_config("{\"version\":250,\"variant\":\"T16\",\"global\":{},\"banks\":[]}");
    cm.Init();
    // Should reset to defaults; version stamp on save is CURRENT_VERSION
    TEST_ASSERT_EQUAL_UINT8(201, cm.Global().version);
    TEST_ASSERT_EQUAL_UINT8(0, cm.Global().mode);          // default
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_version_migrated_to_201);
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
    RUN_TEST(test_v200_migrated_to_v201);
    RUN_TEST(test_v200_migration_serialized_output_has_variant);
    RUN_TEST(test_v201_matching_variant_passes_through);
    RUN_TEST(test_v201_variant_mismatch_falls_back_to_defaults);
    RUN_TEST(test_future_version_falls_back_to_defaults);
    return UNITY_END();
}
