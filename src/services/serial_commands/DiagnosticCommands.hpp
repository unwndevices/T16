#pragma once

namespace t16
{

// Forward declarations
class SerialCommandManager;
class ModeManager;

} // namespace t16

class ConfigManager;
class MidiProvider;
class Keyboard;
class TouchSlider;

namespace t16
{

void registerDiagnosticCommands(
    SerialCommandManager& mgr,
    ConfigManager& config,
    ModeManager& mode,
    MidiProvider& midi,
    Keyboard& keyboard,
    TouchSlider& slider
);

} // namespace t16
