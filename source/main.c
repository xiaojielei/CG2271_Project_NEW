#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "actuator_driver.h"
#include "sensor.h"
#include "uart_bridge.h"

#define UART_BRIDGE_BAUDRATE 9600u

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    static SensorData_t gSensorData;
    SemaphoreHandle_t sensorDataMutex = xSemaphoreCreateMutex();
    configASSERT(sensorDataMutex != NULL);

    Actuators_Init();
    Sensors_Init(&gSensorData, sensorDataMutex);

    UART_Bridge_Init(UART_BRIDGE_BAUDRATE);
    UART_Bridge_SetSensorDataHandle(&gSensorData, sensorDataMutex);

    xTaskCreate(Sensor_Task, "SensorTask", configMINIMAL_STACK_SIZE + 256, NULL, 2, NULL);
    xTaskCreate(Actuator_Task, "ActuatorTask", configMINIMAL_STACK_SIZE + 256, NULL, 1, NULL);
    UART_Bridge_StartTasks(3, 2);

    vTaskStartScheduler();
    for (;;) {
        // Should never reach here.
    }
}
