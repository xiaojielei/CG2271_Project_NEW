/*
 * sensor_manager_task.c
 *
 *  Created on: Oct 29, 2025
 *      Author: 86178
 */


#include "sensor_driver.h"

// Private function prototypes
static void Read_All_Sensors(SensorData_t *data);
static void Process_Sensor_Data(SensorData_t *data);
static void Log_Sensor_Readings(SensorData_t *data);

void Sensor_Polling_Task(void *pvParameters) {
    SensorData_t sensor_data;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t notification_value;

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

        // Get DHT11 data via ESP32 (requires UART mutex)
        if(xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(100))) {
            // Note: DHT11 reading is handled by ESP32 communication task
            // We'll use the last received values
            sensor_data.temperature = current_sensor_data.temperature;
            sensor_data.humidity = current_sensor_data.humidity;
            xSemaphoreGive(xUARTMutex);
        }

        // Add timestamp
        sensor_data.timestamp = xTaskGetTickCount();

        // Process sensor data (filtering, validation)
        Process_Sensor_Data(&sensor_data);

        // Send to communication task
        if(xQueueSend(xSensorQueue, &sensor_data, 0) != pdPASS) {
            // Queue full - implement error handling
            // Could log error or use overwrite option
        }

        // Log readings for debugging (optional)
        Log_Sensor_Readings(&sensor_data);

        // Wait for next polling interval (2 seconds)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
}

static void Read_All_Sensors(SensorData_t *data) {
    // This function provides a single call to read all sensors
    data->water_level = Read_WaterLevel();
    data->light_intensity = Read_Photoresistor();
    data->timestamp = xTaskGetTickCount();
}

static void Process_Sensor_Data(SensorData_t *data) {
    // Simple moving average filter for water level (reduce noise)
    static uint32_t water_level_history[5] = {0};
    static uint8_t history_index = 0;
    static uint8_t history_count = 0;

    // Update history
    water_level_history[history_index] = data->water_level;
    history_index = (history_index + 1) % 5;
    if (history_count < 5) history_count++;

    // Calculate moving average
    uint32_t sum = 0;
    for (int i = 0; i < history_count; i++) {
        sum += water_level_history[i];
    }
    data->water_level = sum / history_count;

    // Validate sensor readings
    if (data->water_level > 100) {
        data->water_level = 100; // Clamp to maximum
    }

    if (data->light_intensity > 4095) {
        data->light_intensity = 4095; // Clamp to maximum ADC value
    }
}

static void Log_Sensor_Readings(SensorData_t *data) {
    // This function can be used for debugging
    // In a real system, you might send this to a log or display

    // Example: Print via UART if debugging is enabled
    #ifdef SENSOR_DEBUG
    if(xSemaphoreTake(xUARTMutex, pdMS_TO_TICKS(50))) {
        // Print sensor readings (implementation depends on your UART setup)
        // PRINTF("Water: %d%%, Light: %d, Temp: %.1fC, Hum: %.1f%%\r\n",
        //        data->water_level, data->light_intensity,
        //        data->temperature, data->humidity);
        xSemaphoreGive(xUARTMutex);
    }
    #endif
}

// Function to update DHT11 data from ESP32
void Update_DHT11_Data(float temperature, float humidity) {
    current_sensor_data.temperature = temperature;
    current_sensor_data.humidity = humidity;
}

// Function to get current sensor readings (for other tasks)
void Get_Current_Sensor_Readings(SensorData_t *data) {
    if (data != NULL) {
        memcpy(data, &current_sensor_data, sizeof(SensorData_t));
    }
}

// Function to check if plant needs immediate attention
uint8_t Check_Plant_Needs_Attention(SensorData_t *data) {
    uint8_t alert_level = 0; // 0=normal, 1=attention, 2=critical

    if (data->water_level < WATER_LEVEL_CRITICAL) {
        alert_level = 2; // Critical - plant needs water immediately
    } else if (data->water_level < WATER_LEVEL_LOW) {
        alert_level = 1; // Attention - water level low
    } else if (data->light_intensity < LIGHT_DARK_THRESHOLD) {
        alert_level = 1; // Attention - too dark
    } else if (data->temperature > 35.0f || data->temperature < 10.0f) {
        alert_level = 1; // Attention - temperature out of range
    }

    return alert_level;
}

// Task creation function (to be called from main)
void Create_Sensor_Task(void) {
    xTaskCreate(Sensor_Polling_Task,
                "SensorTask",
                configMINIMAL_STACK_SIZE + 256,
                NULL,
                2,  // Priority
                &xSensorTaskHandle);
}
