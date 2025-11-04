#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_port.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "actuator_driver.h"
#include "sensor.h"
#include "uart_bridge.h"

#define WATER_LEVEL_PIN       0u  // PTC0 -> ADC0_SE14
#define PHOTORESISTOR_PIN     20u // PTE20 -> ADC0_SE0

static const uint32_t kWaterLevelWetThreshold    = 1800u;
static const uint32_t kPhotoresistorBrightLimit  = 5u;
static const uint32_t kDHT11TemperatureThreshold = 40;
static const uint32_t kDHT11HumidityThreshold = 50;

static SemaphoreHandle_t xWaterLevelSemaphore;
static SemaphoreHandle_t xSensorDataMutex;
static SensorData_t *gSensorData;
static volatile uint32_t gLatestWaterLevel;

//Init both sensors
static void initSensors(void) {
  NVIC_DisableIRQ(ADC0_IRQn);
  NVIC_ClearPendingIRQ(ADC0_IRQn);

    // Enable ADC clock gating (SIM_SCGC6)
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    // Enable clock for PORTE & PORTC
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // Configure photoresistor ADC pin (PTE22 - ADC0_SE3)
    PORTE->PCR[PHOTORESISTOR_PIN] &= ~PORT_PCR_MUX_MASK;
    PORTE->PCR[PHOTORESISTOR_PIN] |= PORT_PCR_MUX(0);

    // Configure water level sensor ADC pin (PTC0 - ADC0_SE14)
  PORTC->PCR[WATER_LEVEL_PIN] &= ~PORT_PCR_MUX_MASK;
  PORTC->PCR[WATER_LEVEL_PIN] |= PORT_PCR_MUX(0);

  // Configure the ADC, Enable ADC interrupt
  //ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(14);// Start interrupt-based conversion on channel 14 (PTC0)
  ADC0->SC1[0] &= ~ADC_SC1_DIFF_MASK;
  ADC0->SC1[0] |= ADC_SC1_DIFF(0b0);

    // Configure ADC0_CFG1 for 12-bit resolution
    ADC0->CFG1 &= ~ADC_CFG1_MODE_MASK;
    ADC0->CFG1 |= ADC_CFG1_MODE(0b01);

    // Use software trigger
    ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;

    // Use VALTH and VALTL
    ADC0->SC2 &= ~ADC_SC2_REFSEL_MASK;
    ADC0->SC2 |= ADC_SC2_REFSEL(0b01);

    // Don't use averaging
    ADC0->SC3 &= ~ADC_SC3_AVGE_MASK;
    ADC0->SC3 |= ADC_SC3_AVGE(0);

    // Use continuous conversion
  ADC0->SC3 &= ~ADC_SC3_ADCO_MASK;
  ADC0->SC3 |= ADC_SC3_ADCO(0);

    NVIC_SetPriority(ADC0_IRQn, 192);
    NVIC_EnableIRQ(ADC0_IRQn);
}
static inline uint32_t adc_read_blocking(uint8_t ch) {
    // wait until converter is idle
    while (ADC0->SC2 & ADC_SC2_ADACT_MASK) { }
    // start conversion on SC1[0] with AIEN=0
    ADC0->SC1[0] = (ADC0->SC1[0] & ~(ADC_SC1_ADCH_MASK | ADC_SC1_AIEN_MASK)) | ADC_SC1_ADCH(ch);
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK)) { }
    return ADC0->R[0];
}

void Sensors_Init(SensorData_t *sharedData, SemaphoreHandle_t dataMutex) {
    gSensorData = sharedData;
    xSensorDataMutex = dataMutex;
    if (gSensorData) {
        memset(gSensorData, 0, sizeof(*gSensorData));
    }

    xWaterLevelSemaphore = xSemaphoreCreateBinary();
    configASSERT(xWaterLevelSemaphore != NULL);

    gLatestWaterLevel = 0u;
    initSensors();
}

 void ADC0_IRQHandler(void) {
     NVIC_ClearPendingIRQ(ADC0_IRQn);
     BaseType_t hpw = pdFALSE;

     if (ADC0->SC1[0] & ADC_SC1_COCO_MASK) {
         uint32_t adcValue = ADC0->R[0];
         gLatestWaterLevel = adcValue;

         xSemaphoreGiveFromISR(xWaterLevelSemaphore, &hpw);
         portYIELD_FROM_ISR(hpw);
// restart conversion on channel 14 (PTC0)
         //ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(14);
     }
 }
/*
void Sensor_Task(void *pvParameters) {
    while (1) {
        sensorData.light_intensity = ReadPhotoresistor();
        PRINTF("water level: %d, light_intensity: %d\n", &sensorData.water_level, &sensorData.light_intensity);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
void Sensor_Task(void *pvParameters) {
     while (1) {
         sensorData.light_adc = ReadPhotoresistor();
         PRINTF("water_adc: %u, light_adc: %u\r\n",
                (unsigned)sensorData.water_adc,
                (unsigned)sensorData.light_adc);
         vTaskDelay(pdMS_TO_TICKS(2000));
     }
 }*/
 void Sensor_Task(void *pvParameters) {
     (void)pvParameters;
     TickType_t lastTelemetryTick = xTaskGetTickCount();
     for (;;) {
         // 1) Start water (PTC0 = ADC0_SE14) with interrupt enabled
         while (ADC0->SC2 & ADC_SC2_ADACT_MASK) { }
         ADC0->SC1[0] = (ADC0->SC1[0] & ~ADC_SC1_ADCH_MASK) | ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(14);

         // 2) Wait for ISR to signal completion
         xSemaphoreTake(xWaterLevelSemaphore, pdMS_TO_TICKS(10)); // small timeout ok

         // 3) Read light (PTE22 = ADC0_SE3) with blocking, no interrupt
         uint32_t lightRaw = adc_read_blocking(0);
         uint32_t waterRaw = gLatestWaterLevel;

         if (gSensorData && xSensorDataMutex) {
             if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                 gSensorData->water_level = waterRaw;
                 gSensorData->light_intensity = lightRaw;
                 xSemaphoreGive(xSensorDataMutex);
             }
         }

         // 4) Print actual values
         PRINTF("water_adc: %u, light_adc: %u\r\n",
                (unsigned)waterRaw,
                (unsigned)lightRaw);

         if (gSensorData && xSensorDataMutex) {
             TickType_t now = xTaskGetTickCount();
             if ((now - lastTelemetryTick) >= pdMS_TO_TICKS(2000)) {
                 SensorData_t snapshot;
                 if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                     snapshot = *gSensorData;
                     xSemaphoreGive(xSensorDataMutex);
                     UART_Bridge_SendSensorTelemetry(&snapshot);
                 }
                 lastTelemetryTick = now;
             }
         }

         vTaskDelay(pdMS_TO_TICKS(200)); // ~5 Hz
     }
 }

/*
void Actuator_Task() {
  while (1) {
    SDK_DelayAtLeastUs(10000U, SystemCoreClock);
    Set_LED_Intensity(sensorData.light_intensity);

    //TODO: Add DHt11 data into below condition
    if (sensorData.water_level >= water_level_wet && sensorData.light_intensity >= photoresistor_bright) {
      Play_Music(MUSIC_SAD);
    } else {
      PRINTF("HAPPYMUSIC");
      //Play_Music(MUSIC_HAPPY);
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}*/

void Actuator_Task(void *pvParameters) {
    (void)pvParameters;
    SensorData_t dataSnapshot = {0};
    while (1) {
        SDK_DelayAtLeastUs(10000U, SystemCoreClock);

        if (gSensorData && xSensorDataMutex) {
            if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                dataSnapshot = *gSensorData;
                xSemaphoreGive(xSensorDataMutex);
            }
        }

        // Map 0..4095 ADC to 0..255 PWM
        uint32_t pwmValue = (dataSnapshot.light_intensity * 255u) / 4095u;
        uint8_t pwm = 0;
        if (gSensorData->light_intensity <= 5) pwm = 0;
        else if (gSensorData->light_intensity >= 30) pwm = 255;
        else pwm = (gSensorData->light_intensity - 5) * 255 / (30 - 5);

        Set_LED_Intensity(pwm);

        bool water_is_wet   = (dataSnapshot.water_level    >= kWaterLevelWetThreshold);
        bool light_is_bright= (dataSnapshot.light_intensity <= kPhotoresistorBrightLimit);
        bool temperature_is_high = (dataSnapshot.temperature <= kDHT11TemperatureThreshold);
        bool humidity_is_high = (dataSnapshot.humidity <= kDHT11HumidityThreshold);

        if (water_is_wet && light_is_bright && temperature_is_high && humidity_is_high) {
            Play_Music(MUSIC_HAPPY);
        } else {
            Play_Music(MUSIC_SAD);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


void Sensor_UpdateRemoteReadings(float temperature, float humidity) {
    if (!gSensorData || !xSensorDataMutex) {
        return;
    }

    if (xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        gSensorData->temperature = temperature;
        gSensorData->humidity = humidity;
        xSemaphoreGive(xSensorDataMutex);
    }
}
