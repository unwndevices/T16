#include <Arduino.h>
#include "AppEngine.hpp"

static t16::AppEngine engine;

void setup()
{
    engine.init();
}

void loop()
{
    engine.update();
}
