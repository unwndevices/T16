#ifndef PERFORMANCE_HPP
#define PERFORMANCE_HPP
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

unsigned long loopCount = 0;
unsigned long lastLoopTime = 0;
float loopRate = 0.0f;
float core0Load = 0.0f;
float core1Load = 0.0f;

void updatePerformance()
{
    loopCount++;
    unsigned long currentTime = millis();
    if (currentTime - lastLoopTime >= 1000)
    {
        loopRate = (float)loopCount / (currentTime - lastLoopTime) * 1000.0f;
        lastLoopTime = currentTime;
        loopCount = 0;
        log_d("Loop Rate: %f Hz, Core 0 Load: %f%%, Core 1 Load: %f%%", loopRate, core0Load, core1Load);
    }
}

void calculateCoreLoads()
{
    TaskHandle_t idleTask0 = xTaskGetIdleTaskHandleForCPU(0);
    TaskHandle_t idleTask1 = xTaskGetIdleTaskHandleForCPU(1);
    uint32_t idleStack0 = uxTaskGetStackHighWaterMark(idleTask0);
    uint32_t idleStack1 = uxTaskGetStackHighWaterMark(idleTask1);
    core0Load = 100.0f - (float)idleStack0 / configMINIMAL_STACK_SIZE * 100.0f;
    core1Load = 100.0f - (float)idleStack1 / configMINIMAL_STACK_SIZE * 100.0f;
}

#endif// PERFORMANCE_HPP
