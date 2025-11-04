#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

typedef struct {
    uint32_t water_level;
    uint32_t light_intensity;
    float temperature;
    float humidity;
} SensorData_t;

void Sensors_Init(SensorData_t *sharedData, SemaphoreHandle_t dataMutex);
void Sensor_Task(void *pvParameters);
void Actuator_Task(void *pvParameters);
void Sensor_UpdateRemoteReadings(float temperature, float humidity);

#endif /* SENSOR_H_ */
