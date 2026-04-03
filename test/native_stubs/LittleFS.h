#pragma once
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>

// Minimal File stub that reads/writes to temp files
class FakeFile
{
public:
    FILE* fp = nullptr;
    bool valid = false;

    operator bool() const { return valid && fp != nullptr; }

    size_t write(const uint8_t* buf, size_t size)
    {
        if (!fp) return 0;
        return fwrite(buf, 1, size, fp);
    }

    int read()
    {
        if (!fp) return -1;
        return fgetc(fp);
    }

    size_t readBytes(char* buf, size_t len)
    {
        if (!fp) return 0;
        return fread(buf, 1, len, fp);
    }

    void close()
    {
        if (fp) { fclose(fp); fp = nullptr; }
        valid = false;
    }

    size_t size()
    {
        if (!fp) return 0;
        long pos = ftell(fp);
        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        fseek(fp, pos, SEEK_SET);
        return (size_t)sz;
    }
};

class FakeLittleFS
{
public:
    const char* testDir = "/tmp/t16_test_fs";

    bool begin(bool formatOnFail = false)
    {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", testDir);
        system(cmd);
        return true;
    }

    FakeFile open(const char* path, const char* mode, bool create = false)
    {
        FakeFile f;
        char fullPath[512];
        // Replace leading / for flat file storage
        const char* name = path;
        if (name[0] == '/') name++;
        snprintf(fullPath, sizeof(fullPath), "%s/%s", testDir, name);
        f.fp = fopen(fullPath, mode);
        f.valid = (f.fp != nullptr);
        if (!f.valid && create)
        {
            f.fp = fopen(fullPath, "w+");
            f.valid = (f.fp != nullptr);
        }
        return f;
    }

    void cleanup()
    {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", testDir);
        system(cmd);
    }
};

extern FakeLittleFS LittleFS;
