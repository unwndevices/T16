#include "SerialCommandManager.hpp"
#include <string.h>

namespace t16
{

static void cmdHelp(const char* args);
static SerialCommandManager* sInstance = nullptr;

SerialCommandManager::SerialCommandManager()
{
    sInstance = this;
    registerCommand("help", "Show available commands", cmdHelp);
}

void SerialCommandManager::registerCommand(const char* name, const char* help, CommandHandler handler)
{
    if (commandCount_ >= MAX_COMMANDS)
    {
        Serial.println("Error: max commands reached");
        return;
    }
    commands_[commandCount_] = {name, help, handler};
    commandCount_++;
}

void SerialCommandManager::update()
{
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            if (inputPos_ > 0)
            {
                inputBuffer_[inputPos_] = '\0';
                dispatch(inputBuffer_);
                inputPos_ = 0;
            }
        }
        else if (inputPos_ < INPUT_BUFFER_SIZE - 1)
        {
            inputBuffer_[inputPos_++] = c;
        }
    }
}

void SerialCommandManager::dispatch(const char* line)
{
    // Split command name from args
    const char* space = strchr(line, ' ');
    size_t nameLen = space ? (size_t)(space - line) : strlen(line);
    const char* args = space ? space + 1 : "";

    for (uint8_t i = 0; i < commandCount_; i++)
    {
        if (strlen(commands_[i].name) == nameLen &&
            strncmp(commands_[i].name, line, nameLen) == 0)
        {
            commands_[i].handler(args);
            return;
        }
    }

    Serial.printf("Unknown command: %s\n", line);
    Serial.println("Type 'help' for list.");
}

void SerialCommandManager::printHelp()
{
    Serial.println("--- Available Commands ---");
    for (uint8_t i = 0; i < commandCount_; i++)
    {
        Serial.printf("  %-12s %s\n", commands_[i].name, commands_[i].help);
    }
    Serial.println("--------------------------");
}

static void cmdHelp(const char* args)
{
    if (sInstance)
    {
        sInstance->printHelp();
    }
}

} // namespace t16
