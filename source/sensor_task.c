/*
 * sensor_manager_task.c
 *
 *  Created on: Oct 29, 2025
 *      Author: 86178
 */


#include "sensor_driver.h"
#include <stdio.h>

static void Log_Sensor_Readings(SensorData_t *data);
void Get_Current_Sensor_Readings(SensorData_t *data);

void Sensor_Polling_Task(void *pvParameters) {
    SensorData_t sensor_data;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    // Initialize sensor data structure
    memset(&sensor_data, 0, sizeof(SensorData_t));
    memset(&current_sensor_data, 0, sizeof(SensorData_t));

    for(;;) {
        // Read polling sensors (photoresistor)
        sensor_data.light_intensity = Read_Photoresistor();

        // Try to get water level data from ISR (non-blocking)
        if(xSemaphoreTake(xWaterLevelSemaphore, 0)) {
            sensor_data.water_level = Get_Last_WaterLevel();
        } else {
            // If no recent interrupt, read water level directly
            sensor_data.water_level = Read_WaterLevel();
        }

        Log_Sensor_Readings(&sensor_data);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
}

static void Log_Sensor_Readings(SensorData_t *data) {
    if(xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(50))) {
        // Print sensor readings, humidity and temperature are printed by ESP32 task
        //PRINTF("Water: %d%%, Light: %d\n", data->water_level, data->light_intensity);

        xSemaphoreGive(xUARTMutex);
    }

}

// Function to get current sensor readings (for other tasks)
void Get_Current_Sensor_Readings(SensorData_t *data) {
    if (data != NULL) {
        memcpy(data, &current_sensor_data, sizeof(SensorData_t));
    }
}

void Create_Sensor_Task(void) {
    xTaskCreate(Sensor_Polling_Task,
                "SensorTask",
                configMINIMAL_STACK_SIZE + 256,
                NULL,
                2,  // Priority
                &xSensorTaskHandle);
}
