#include "Button.hpp"

Button::Button(int pin, int id, int debounceTime)
    : pin(pin),
      id(id),
      debounceTime(debounceTime),
      lastDebounceTime(0),
      state(IDLE), prevState(IDLE),
      pressStartTime(0),
      elapsedTime(0),
      longPressTime(650),
      clickTime(260),
      previousReading(false),
      longPressFlag(false) {}

void Button::SetLongPressTime(unsigned long time)
{
    longPressTime = time;
}

void Button::Init(int id)
{
    log_d("Button %d initialized", id);
    this->id = id;
    pinMode(pin, INPUT_PULLUP);
    state = IDLE;
    prevState = IDLE;
    pressStartTime = 0;
    elapsedTime = 0;
    longPressFlag = false;
}

void Button::Init()
{
    pinMode(pin, INPUT_PULLUP);
}

void Button::Start()
{
    xTaskCreatePinnedToCore(Button::taskUpdate, "button", 1024 * 2, this, 3, &_task, 0);
}

void Button::taskUpdate(void *pvParameters)
{
    Button *button = static_cast<Button *>(pvParameters);
    while (1)
    {
        button->Update();
        vTaskDelay(pdMS_TO_TICKS(4));
    }
}

bool Button::GetRaw() { return reading; }

void Button::Update()
{
    reading = (bool)(!digitalRead(pin));

    if (reading != previousReading)
    {
        lastDebounceTime = millis();
        previousReading = reading;
        return;
    }

    if ((millis() - lastDebounceTime) > debounceTime)
    {
        switch (state)
        {
        case IDLE:
            if (reading)
            {
                state = PRESSED;
                pressStartTime = millis();
                elapsedTime = 0;
            }
            break;

        case PRESSED:
            if (reading)
            {
                elapsedTime = millis() - pressStartTime;
                if (elapsedTime > longPressTime)
                {
                    state = LONG_PRESSED;
                }
            }
            else if (!reading)
            {
                if ((millis() - pressStartTime) < clickTime)
                {
                    state = CLICKED;
                }
                else
                {
                    state = RELEASED;
                }
            }
            break;

        case RELEASED:
            if (reading)
            {
                state = PRESSED;
                pressStartTime = millis();
            }
            else
            {
                state = IDLE;
            }
            break;

        case LONG_PRESSED:
            if (!reading)
            {
                state = LONG_RELEASED;
            }
            break;

        case LONG_RELEASED:
            if (reading)
            {
                state = PRESSED;
                pressStartTime = millis();
            }
            else
            {
                state = IDLE;
            }
            break;

        case CLICKED:
            state = IDLE;
            break;
        }

        if (prevState != state)
        {
            prevState = state;
            onStateChanged.Emit(id, state);
        }
    }
}

Button::State Button::GetState()
{
    return state;
}

bool Button::IsPressed()
{
    return reading;
}

float Button::GetHoldTimeNormalized()
{
    return (float)elapsedTime / (float)longPressTime;
}
