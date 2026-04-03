#include "Arduino.h"

static unsigned long fake_millis = 0;

unsigned long millis()
{
    return fake_millis;
}

void set_fake_millis(unsigned long ms)
{
    fake_millis = ms;
}
