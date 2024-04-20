#ifndef DATAMANAGER_HPP
#define DATAMANAGER_HPP

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <StreamUtils.h>

class DataManager
{
public:
    DataManager(const char *filename) : filename(filename) {}

    void Init()
    {
        if (!LittleFS.begin())
        {
            log_d("An Error has occurred while mounting LittleFS");
        }
    }

    template <typename T>
    void SaveVar(T var, const char *name)
    {
        JsonDocument doc = LoadJsonDocument();
        doc[name] = var;
        SaveJsonDocument(doc);
    }

    template <typename T>
    void SaveArray(T *array, const char *name, uint8_t size)
    {
        JsonDocument doc = LoadJsonDocument();
        JsonArray jsonArray = doc[name].as<JsonArray>();
        if (jsonArray.isNull())
        {
            jsonArray = doc[name].to<JsonArray>();
        }
        else
        {
            jsonArray.clear(); // Clear existing array to overwrite
        }
        for (uint8_t i = 0; i < size; i++)
        {
            jsonArray.add(array[i]);
        }
        SaveJsonDocument(doc);
    }

    template <typename T>
    bool LoadVar(T &var, const char *name)
    {
        JsonDocument doc = LoadJsonDocument();
        if (doc.isNull())
            return false;

        var = doc[name].as<T>();
        return true;
    }

    template <typename T>
    bool LoadArray(T *array, const char *name, uint8_t size)
    {
        JsonDocument doc = LoadJsonDocument();
        if (doc.isNull())
            return false;

        JsonArray jsonArray = doc[name].as<JsonArray>();
        for (uint8_t i = 0; i < size; i++)
        {
            array[i] = jsonArray[i].as<T>();
        }
        return true;
    }

    void SaveBanksArray(JsonArray &banksArray)
    {
        JsonDocument doc = LoadJsonDocument();
        doc["banks"] = banksArray;
        SaveJsonDocument(doc);
    }

    bool LoadBanksArray(JsonArray &banksArray)
    {
        JsonDocument doc = LoadJsonDocument();
        if (doc.isNull())
            return false;

        banksArray = doc["banks"].as<JsonArray>();
        return true;
    }

    size_t SerializeToBuffer(char *buffer, size_t size)
    {
        JsonDocument doc = LoadJsonDocument();
        return serializeJson(doc, buffer, size);
    }

    void DeserializeFromBuffer(char *buffer)
    {
        JsonDocument doc;
        deserializeJson(doc, buffer);
        SaveJsonDocument(doc);
    }

    void Print()
    {
        JsonDocument doc = LoadJsonDocument();
        if (!doc.isNull())
            serializeJson(doc, Serial);
    }

    bool HasChanged()
    {
        if (hasChanged)
        {
            hasChanged = false;
            return true;
        }
        else
        {
            return false;
        }
    }

    JsonDocument LoadJsonDocument()
    {
        File file = LittleFS.open(filename, "r");
        if (!file)
        {
            log_d("Failed to open file for reading");
            return JsonDocument();
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error)
        {
            log_d("deserializeJson() failed: %s", error.c_str());
            return JsonDocument();
        }

        return doc;
    }

    void SaveJsonDocument(JsonDocument &doc, bool createIfNotExists = true)
    {
        File file = LittleFS.open(filename, "w", createIfNotExists);
        if (!file)
        {
            log_d("Failed to open file for writing");
            return;
        }

        serializeJson(doc, file);
        file.close();
        hasChanged = true;
    }

private:
    const char *filename;
    bool hasChanged = false;
};

#endif // DATAMANAGER_HPP
