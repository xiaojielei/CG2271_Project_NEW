#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "board.h"
#include "fsl_port.h"

// Sensor Data Structure
typedef struct {
    uint32_t water_level;      // 0-100%
    uint32_t light_intensity;  // 0-1023 (ADC value)
    float temperature;         // Celsius
    float humidity;            // Percentage
} SensorData_t;

//Global Variables
extern uint32_t water_level_dry;
extern uint32_t water_level_wet;
extern uint32_t photoresistor_dark;
extern uint32_t photoresistor_bright;

extern QueueHandle_t xSensorQueue;
extern SemaphoreHandle_t xWaterLevelSemaphore;
extern SemaphoreHandle_t xUARTMutex;
extern TaskHandle_t xSensorTaskHandle;

extern uint32_t last_water_level;
extern TickType_t last_interrupt_time;
extern SensorData_t current_sensor_data;

// Hardware Pin Definitions
#define WATER_LEVEL_PIN       4  // PTA4
#define PHOTORESISTOR_PIN     0   // ADC0_DP0
#define WATER_LEVEL_PORT      PORTA
#define WATER_LEVEL_GPIO      GPIOA
#define WATER_LEVEL_IRQ       PORTA_IRQn

// ADC Configuration
#define ADC_BASE              ADC0
#define ADC_CHANNEL_GROUP     0
#define ADC_RESOLUTION        12  // 12-bit resolution

void Sensors_Init(void);

uint32_t Read_Photoresistor(void);
uint32_t Read_WaterLevel(void);
uint32_t Get_Last_WaterLevel(void);

//Get latest data of sensors
void Get_Current_Sensor_Readings(SensorData_t *data);

void Create_Sensor_Task(void);

#endif /* SENSOR_DRIVER_H */
