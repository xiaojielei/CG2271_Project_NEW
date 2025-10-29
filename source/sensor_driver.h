#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "board.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_clock.h"

// Sensor Data Structure
typedef struct {
    uint32_t water_level;      // 0-100%
    uint32_t light_intensity;  // 0-1023 (ADC value)
    float temperature;         // Celsius
    float humidity;            // Percentage
    TickType_t timestamp;
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

// Water Level Thresholds
#define WATER_LEVEL_CRITICAL  20  // Below 20% - critical
#define WATER_LEVEL_LOW       40  // Below 40% - low
#define WATER_LEVEL_HIGH      80  // Above 80% - high

// Photoresistor Thresholds
#define LIGHT_DARK_THRESHOLD  300  // Below 300 - too dark
#define LIGHT_BRIGHT_THRESHOLD 800 // Above 800 - too bright

// Hardware Pin Definitions
#define WATER_LEVEL_PIN       4  // PTA4
#define PHOTORESISTOR_PIN     0U   // ADC0_DP0
#define WATER_LEVEL_PORT      PORTA
#define WATER_LEVEL_GPIO      GPIOA
#define WATER_LEVEL_IRQ       PORTA_IRQn

// ADC Configuration
#define ADC_BASE              ADC0
#define ADC_CHANNEL_GROUP     0U
#define ADC_RESOLUTION        12U  // 12-bit resolution

// Function Prototypes
void Sensors_Init(void);
void init_WaterLevel_ISR(void);
void ADC_Config(void);
uint32_t Read_WaterLevel(void);
uint32_t Read_Photoresistor(void);
void Calibrate_WaterLevel(void);
void Sensor_SelfTest(void);
uint32_t Get_Last_WaterLevel();

// Interrupt Handler
void PORTA_IRQHandler(void);

void Create_Sensor_Task(void);
void Sensor_Polling_Task(void *pvParameters);
void Update_DHT11_Data(float temperature, float humidity);

#endif /* SENSOR_DRIVER_H */
