/*
 * global_definitions.c
 *
 *  Created on: Oct 29, 2025
 *      Author: 86178
 */


#include "sensor_driver.h"

// Define all global variables here (ONCE in the entire project)

// Global variables for sensor calibration
uint32_t water_level_dry = 0;    // ADC value when dry
uint32_t water_level_wet = 4095; // ADC value when fully submerged
uint32_t photoresistor_dark = 0;
uint32_t photoresistor_bright = 4095;

// FreeRTOS objects
QueueHandle_t xSensorQueue = NULL;
SemaphoreHandle_t xWaterLevelSemaphore = NULL;
SemaphoreHandle_t xUARTMutex = NULL;
TaskHandle_t xSensorTaskHandle = NULL;

// Water level state tracking
uint32_t last_water_level = 0;
TickType_t last_interrupt_time = 0;

// Sensor data storage
SensorData_t current_sensor_data;
