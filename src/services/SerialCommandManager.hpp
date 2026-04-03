#pragma once

#include <Arduino.h>
#include <array>
#include <stdint.h>

namespace t16
{

class SerialCommandManager
{
public:
    using CommandHandler = void(*)(const char* args);

    SerialCommandManager();

    void registerCommand(const char* name, const char* help, CommandHandler handler);
    void update();
    void printHelp();

private:
    struct Command
    {
        const char* name;
        const char* help;
        CommandHandler handler;
    };

    static constexpr uint8_t MAX_COMMANDS = 16;
    static constexpr uint8_t INPUT_BUFFER_SIZE = 64;

    std::array<Command, MAX_COMMANDS> commands_;
    uint8_t commandCount_ = 0;
    char inputBuffer_[INPUT_BUFFER_SIZE] = {0};
    uint8_t inputPos_ = 0;

    void dispatch(const char* line);
};

} // namespace t16
