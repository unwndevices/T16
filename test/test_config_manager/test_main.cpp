#include "Arduino.h"
#include "LittleFS.h"

// Include implementation sources for native test linking
// (PlatformIO native tests need sources linked directly)
#include "../native_stubs/Arduino.cpp"
#include "../native_stubs/LittleFS.cpp"
#include "../../src/ConfigManager.cpp"

#include <unity.h>
#include "ConfigManager.hpp"
#include "../../src/SysExProtocol.hpp"

static ConfigManager cm;

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

// --- Default values after Init ---

void test_init_default_mode(void)
{
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(0, cm.Global().mode);
}

void test_init_default_sensitivity(void)
{
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(1, cm.Global().sensitivity);
}

void test_init_default_brightness(void)
{
    cm.Init();
    TEST_ASSERT_EQUAL_UINT8(6, cm.Global().brightness);
}

void test_init_default_version(void)
{
    cm.Init();
    TEST_ASSERT_EQUAL_UINT16(201, cm.Global().version);
}

// --- SetGlobalParam ---

void test_set_global_param_mode(void)
{
    cm.Init();
    cm.SetGlobalParam(SysEx::FIELD_GLOBAL_MODE, 3);
    TEST_ASSERT_EQUAL_UINT8(3, cm.Global().mode);
}

void test_set_global_param_sets_dirty(void)
{
    cm.Init();
    TEST_ASSERT_FALSE(cm.IsDirty());
    cm.SetGlobalParam(SysEx::FIELD_GLOBAL_MODE, 3);
    TEST_ASSERT_TRUE(cm.IsDirty());
}

void test_set_global_param_brightness(void)
{
    cm.Init();
    cm.SetGlobalParam(SysEx::FIELD_GLOBAL_BRIGHTNESS, 10);
    TEST_ASSERT_EQUAL_UINT8(10, cm.Global().brightness);
}

// --- SetBankParam ---

void test_set_bank_param_channel(void)
{
    cm.Init();
    cm.SetBankParam(0, SysEx::FIELD_BANK_CHANNEL, 5);
    TEST_ASSERT_EQUAL_UINT8(5, cm.Bank(0).channel);
}

void test_set_bank_param_sets_dirty(void)
{
    cm.Init();
    TEST_ASSERT_FALSE(cm.IsDirty());
    cm.SetBankParam(0, SysEx::FIELD_BANK_CHANNEL, 5);
    TEST_ASSERT_TRUE(cm.IsDirty());
}

void test_set_bank_param_invalid_bank(void)
{
    cm.Init();
    // Should not crash or modify anything for invalid bank index
    cm.SetBankParam(BANK_AMT + 1, SysEx::FIELD_BANK_CHANNEL, 5);
    // No assertion needed -- just verifying no crash
    TEST_ASSERT_TRUE(true);
}

// --- SerializeToBuffer ---

void test_serialize_contains_version_201(void)
{
    cm.Init();
    char buffer[2048];
    size_t len = cm.SerializeToBuffer(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);

    TEST_ASSERT_NOT_NULL(strstr(buffer, "\"version\":201"));
}

void test_serialize_contains_global_key(void)
{
    cm.Init();
    char buffer[2048];
    cm.SerializeToBuffer(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\"global\""));
}

void test_serialize_contains_banks_key(void)
{
    cm.Init();
    char buffer[2048];
    cm.SerializeToBuffer(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "\"banks\""));
}

// --- DeserializeFromBuffer ---

void test_deserialize_valid_json(void)
{
    cm.Init();
    const char* json = "{\"global\":{\"version\":200,\"mode\":2,\"sensitivity\":3,"
                        "\"brightness\":8,\"midi_trs\":1,\"trs_type\":0,"
                        "\"passthrough\":1,\"midi_ble\":0,"
                        "\"custom_scale1\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15],"
                        "\"custom_scale2\":[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]},"
                        "\"banks\":[]}";
    bool result = cm.DeserializeFromBuffer(json, strlen(json));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(2, cm.Global().mode);
    TEST_ASSERT_EQUAL_UINT8(3, cm.Global().sensitivity);
    TEST_ASSERT_EQUAL_UINT8(8, cm.Global().brightness);
}

void test_deserialize_invalid_json(void)
{
    cm.Init();
    const char* garbage = "not json at all {{{}}}";
    bool result = cm.DeserializeFromBuffer(garbage, strlen(garbage));
    TEST_ASSERT_FALSE(result);
}

void test_deserialize_missing_global_key(void)
{
    cm.Init();
    // Valid JSON without "global" key falls through to v103 flat-format defaults
    const char* json = "{\"banks\":[]}";
    bool result = cm.DeserializeFromBuffer(json, strlen(json));
    TEST_ASSERT_TRUE(result);
}

// --- CheckIdleFlush ---

void test_idle_flush_not_dirty(void)
{
    cm.Init();
    // Not dirty, should not save (no crash)
    cm.CheckIdleFlush(5000);
    TEST_ASSERT_FALSE(cm.IsDirty());
}

void test_idle_flush_dirty_too_soon(void)
{
    cm.Init();
    set_fake_millis(1000);
    cm.SetGlobalParam(SysEx::FIELD_GLOBAL_MODE, 1);
    TEST_ASSERT_TRUE(cm.IsDirty());
    // Only 500ms elapsed
    cm.CheckIdleFlush(1500);
    TEST_ASSERT_TRUE(cm.IsDirty()); // Still dirty -- not enough time
}

void test_idle_flush_dirty_enough_time(void)
{
    cm.Init();
    set_fake_millis(1000);
    cm.SetGlobalParam(SysEx::FIELD_GLOBAL_MODE, 1);
    TEST_ASSERT_TRUE(cm.IsDirty());
    // 2000ms elapsed
    cm.CheckIdleFlush(3000);
    TEST_ASSERT_FALSE(cm.IsDirty()); // Should have saved
}

// --- SetCCParam ---

void test_set_cc_param(void)
{
    cm.Init();
    cm.SetCCParam(0, 0, 3, 42);
    TEST_ASSERT_EQUAL_UINT8(3, cm.CC(0).channel[0]);
    TEST_ASSERT_EQUAL_UINT8(42, cm.CC(0).id[0]);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_default_mode);
    RUN_TEST(test_init_default_sensitivity);
    RUN_TEST(test_init_default_brightness);
    RUN_TEST(test_init_default_version);
    RUN_TEST(test_set_global_param_mode);
    RUN_TEST(test_set_global_param_sets_dirty);
    RUN_TEST(test_set_global_param_brightness);
    RUN_TEST(test_set_bank_param_channel);
    RUN_TEST(test_set_bank_param_sets_dirty);
    RUN_TEST(test_set_bank_param_invalid_bank);
    RUN_TEST(test_serialize_contains_version_201);
    RUN_TEST(test_serialize_contains_global_key);
    RUN_TEST(test_serialize_contains_banks_key);
    RUN_TEST(test_deserialize_valid_json);
    RUN_TEST(test_deserialize_invalid_json);
    RUN_TEST(test_deserialize_missing_global_key);
    RUN_TEST(test_idle_flush_not_dirty);
    RUN_TEST(test_idle_flush_dirty_too_soon);
    RUN_TEST(test_idle_flush_dirty_enough_time);
    RUN_TEST(test_set_cc_param);
    return UNITY_END();
}
