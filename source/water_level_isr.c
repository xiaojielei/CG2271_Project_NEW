/*
 * water_level_isr.c
 *
 *  Created on: Oct 29, 2025
 *      Author: 86178
 */

#include "sensor_driver.h"
#include "FreeRTOS.h"
#include "semphr.h"

// External semaphore for water level interrupt
extern SemaphoreHandle_t xWaterLevelSemaphore;

// Water level state tracking
extern uint32_t last_water_level = 0;
extern TickType_t last_interrupt_time = 0;

void PORTA_IRQHandler(void) {

	NVIC_ClearPendingIRQ(PORTA_IRQn);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TickType_t current_time;

    // Check if this interrupt is from our water level pin
    if (PORT_GetPinsInterruptFlags(WATER_LEVEL_PORT) & (1U << WATER_LEVEL_PIN)) {

        // Get current time
        current_time = xTaskGetTickCountFromISR();

        // Simple debouncing - ignore interrupts that occur too close together
        if ((current_time - last_interrupt_time) > pdMS_TO_TICKS(50)) {

            // Read current water level
            uint32_t current_water_level = Read_WaterLevel();

            // Only trigger if significant change detected (to avoid noise)
            if (abs((int32_t)current_water_level - (int32_t)last_water_level) > 5) {

                // Update last known water level
                last_water_level = current_water_level;

                // Give semaphore to notify sensor task
                if (xWaterLevelSemaphore != NULL) {
                    xSemaphoreGiveFromISR(xWaterLevelSemaphore, &xHigherPriorityTaskWoken);
                }
            }

            last_interrupt_time = current_time;
        }

        // Clear interrupt flag
        PORT_ClearPinsInterruptFlags(WATER_LEVEL_PORT, (1U << WATER_LEVEL_PIN));
    }

    // Perform context switch if necessary
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	// Write a 1 to clear the ISFR bit (yes, 1, not 0)
	PORTA->ISFR |= (1 << WATER_LEVEL_PIN);
}

// Function to get the last measured water level
uint32_t Get_Last_WaterLevel(void) {
    return last_water_level;
}

// Function to manually trigger water level reading (for testing)
void Trigger_WaterLevel_Reading(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (xWaterLevelSemaphore != NULL) {
        xSemaphoreGiveFromISR(xWaterLevelSemaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
