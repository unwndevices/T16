#ifndef DATAMANAGER_HPP
#define DATAMANAGER_HPP

#include <ArduinoJson.h>
#include <LittleFS.h>

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
        JsonDocument doc;
        File file = LittleFS.open(filename, "r");
        if (!file)
        {
            log_d("Failed to open file for writing, creating file");
            file = LittleFS.open(filename, "w", true);
        }
        deserializeJson(doc, file);
        file.close();

        doc[name] = var;

        file = LittleFS.open(filename, "w");
        serializeJson(doc, file);
        file.close();
    }

    template <typename T>
    void SaveArray(T *array, const char *name, uint8_t size)
    {
        JsonDocument doc;
        File file = LittleFS.open(filename, "r");
        if (!file)
        {
            log_d("Failed to open file for writing, creating file");
            file = LittleFS.open(filename, "w", true);
        }

        deserializeJson(doc, file);
        file.close();

        JsonArray jsonArray = doc[name].to<JsonArray>();
        for (uint8_t i = 0; i < size; i++)
        {
            jsonArray.add(array[i]);
        }

        file = LittleFS.open(filename, "w");
        serializeJson(doc, file);
        file.close();
    }

    template <typename T>
    bool LoadVar(T &var, const char *name)
    {
        File file = LittleFS.open(filename, "r");
        if (!file)
        {
            log_d("Failed to open file for reading");
            return false;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error)
        {
            log_d("deserializeJson() failed: %s", error.c_str());
            return false;
        }

        var = doc[name].as<T>();
        file.close();
        return true;
    }

    template <typename T>
    bool LoadArray(T *array, const char *name, uint8_t size)
    {
        File file = LittleFS.open(filename, "r");
        if (!file)
        {
            log_d("Failed to open file for reading");
            return false;
        }

        // Parse the JSON file and deserialize it into a JSON document
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);

        // Test if parsing succeeds.
        if (error)
        {
            log_d("deserializeJson() failed: %s", error.c_str());
            return false;
        }

        // Extract values
        JsonArray jsonArray = doc[name].as<JsonArray>();
        for (uint8_t i = 0; i < size; i++)
        {
            array[i] = jsonArray[i].as<T>();
        }
        file.close();
        return true;
    }

    size_t SerializeToBuffer(char *buffer)
    {
        File file = LittleFS.open(filename, "r");
        JsonDocument doc;
        deserializeJson(doc, file);
        file.close();
        return serializeJson(doc, buffer, 512);
    }

    void DeserializeFromBuffer(char *buffer)
    {
        File file = LittleFS.open(filename, "w");
        JsonDocument doc;
        deserializeJson(doc, buffer);
        serializeJson(doc, file);
        file.close();
    }

    void Print()
    {
        File file = LittleFS.open(filename, "r");
        if (!file)
        {
            log_d("Failed to open file for reading");
        }

        // Parse the JSON file and deserialize it into a JSON document
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);

        serializeJson(doc, Serial);
        file.close();
    }

private:
    const char *filename;
};

#endif // DATAMANAGER_HPP
